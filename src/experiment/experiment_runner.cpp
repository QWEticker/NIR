#include "experiment/experiment_runner.hpp"
#include "postprocessing/parameter_estimator.hpp"
#include "postprocessing/security_analyzer.hpp"
#include "protocol/heterodyne_detector.hpp"
#include "protocol/qpsk_modulator.hpp"
#include "protocol/quantum_channel.hpp"
#include "attacks/saturation_attack.hpp"
#include "attacks/lo_manipulation_attack.hpp"
#include "attacks/intercept_resend_attack.hpp"
#include "attacks/collective_attack.hpp"
#include "attacks/entangling_cloner_model.cpp"
#include "attacks/intercept_resend_model.cpp"
#include "attacks/lo_manipulation_model.cpp"
#include "attacks/saturation_model.cpp"
#include "json.hpp"
#include <chrono>
#include <iostream>

namespace cvqkd {

using json = nlohmann::json;

ExperimentRunner::ExperimentRunner(ExperimentConfig c,
                                   std::shared_ptr<StatisticsLogger> l)
    : cfg_(std::move(c)), logger_(std::move(l)) {
    // Build attacks & analytical models from cfg_.attacks and cfg_.attack_params_json
    for (size_t i = 0; i < cfg_.attacks.size(); ++i) {
        const std::string &name = cfg_.attacks[i];
        std::string params = "";
        if (i < cfg_.attack_params_json.size()) params = cfg_.attack_params_json[i];

        // attach simulation attack
        if (name == "saturation") {
            double sat = cfg_.base.saturation_level;
            double gain = 1.0;
            if (!params.empty() && params != "\"") {
                try {
                    auto pj = json::parse(params);
                    if (pj.contains("saturation_level")) sat = pj.at("saturation_level");
                    if (pj.contains("gain")) gain = pj.at("gain");
                } catch(...) {}
            }
            attach_attack(std::make_unique<SaturationAttack>(sat, gain));
            attack_models_.push_back(std::make_unique<SaturationModel>(sat, gain));
            attack_names_.push_back("saturation");
            attack_params_.push_back(params);
        } else if (name == "lo" || name == "lo_manipulation" || name == "lo_manipulation_attack") {
            double scale = 1.0;
            if (!params.empty() && params != "\"") {
                try { auto pj = json::parse(params); if (pj.contains("scale")) scale = pj.at("scale"); } catch(...) {}
            }
            attach_attack(std::make_unique<LOManipulationAttack>(scale));
            attack_models_.push_back(std::make_unique<LOManipulationModel>(scale));
            attack_names_.push_back("lo");
            attack_params_.push_back(params);
        } else if (name == "intercept_resend") {
            double meas_eff = 0.8, resend_gain = 1.0;
            if (!params.empty() && params != "\"") {
                try { auto pj = json::parse(params); if (pj.contains("meas_eff")) meas_eff = pj.at("meas_eff"); if (pj.contains("resend_gain")) resend_gain = pj.at("resend_gain"); } catch(...) {}
            }
            attach_attack(std::make_unique<InterceptResendAttack>(meas_eff, resend_gain, cfg_.base.seed + 1000));
            attack_models_.push_back(std::make_unique<InterceptResendModel>(meas_eff, resend_gain));
            attack_names_.push_back("intercept_resend");
            attack_params_.push_back(params);
        } else if (name == "collective") {
            double coupling = 0.3, excess = 0.05;
            if (!params.empty() && params != "\"") {
                try { auto pj = json::parse(params); if (pj.contains("coupling")) coupling = pj.at("coupling"); if (pj.contains("excess_noise")) excess = pj.at("excess_noise"); } catch(...) {}
            }
            attach_attack(std::make_unique<CollectiveAttack>(coupling, excess, cfg_.base.seed + 2000));
            // For analytic model use entangling cloner with extra xi = excess
            attack_models_.push_back(std::make_unique<EntanglingClonerModel>(excess));
            attack_names_.push_back("collective");
            attack_params_.push_back(params);
        } else if (name == "none") {
            // nothing
            attack_names_.push_back("none");
            attack_params_.push_back(params);
            attack_models_.push_back(nullptr);
        } else {
            // unknown — store name and raw params
            attack_names_.push_back(name);
            attack_params_.push_back(params);
            attack_models_.push_back(nullptr);
        }
    }
}

void ExperimentRunner::attach_attack(std::unique_ptr<IAttack> a) {
    attacks_.push_back(std::move(a));
}

void ExperimentRunner::run_one_point(double dist, const std::string& scenario) {
    ProtocolConfig pc = cfg_.base;
    pc.distance_km = dist;
    pc.recompute_T_from_distance();

    QPSKModulator      mod(pc.alpha, pc.seed);
    QuantumChannel     ch(pc);
    HeterodyneDetector det(pc);
    ParameterEstimator est(pc, 0.95);
    SecurityAnalyzer   sec(pc);

    auto t0 = std::chrono::steady_clock::now();

    auto alice = mod.generate(pc.N);
    auto bob_in = ch.transmit(alice.amplitudes);
    auto meas   = det.measure(bob_in);

    for (const auto& a : attacks_) if (a) a->apply(meas);

    // Формируем вектора для оценки: используем ОБЕ квадратуры (X и P)
    VectorR xA(2 * pc.N), yB(2 * pc.N);
    for (std::size_t i = 0; i < pc.N; ++i) {
        xA(i)            = alice.amplitudes[i].real();
        xA(pc.N + i)     = alice.amplitudes[i].imag();
        yB(i)            = meas.X(static_cast<Eigen::Index>(i));
        yB(pc.N + i)     = meas.P(static_cast<Eigen::Index>(i));
    }

    auto e = est.estimate(xA, yB);
    e.T_hat   = std::clamp(e.T_hat, 0.01, 1.0);
    e.xi_hat  = std::max(0.0, e.xi_hat);

    // Apply analytical attack models cumulatively for security analysis
    double xi_effective = e.xi_hat;
    Eigen::Matrix4d custom_cm = Eigen::Matrix4d::Zero();
    bool have_custom_cm = false;
    for (size_t i = 0; i < attack_models_.size(); ++i) {
        const auto& am = attack_models_[i];
        if (!am) continue;
        auto ar = am->computeEffect(pc.T, mod.modulation_variance(), e.xi_hat);
        if (ar.type == cvqkd::AttackResultType::XiAdd) xi_effective += ar.xi_add;
        else if (ar.type == cvqkd::AttackResultType::Covariance) {
            custom_cm = ar.modified_cm;
            have_custom_cm = true;
        }
    }

    // Compute security metrics: if custom CM available compute chi from CM, otherwise use sec.compute with xi_effective
    SecurityMetrics m{};
    if (have_custom_cm) {
        // compute chi from provided CM using HolevoCalculator (in nim namespace)
        // HolevoCalculator is under nir namespace, include via header
        double chi = nir::HolevoCalculator::computeHolevoFromCM(custom_cm);
        // compute I_AB using standard SNR approximation
        // Detector referred noise approx: v_det ~= v_el / eta
        double v_det = pc.v_el / std::max(1e-12, pc.eta);
        double signal = pc.T * mod.modulation_variance();
        double noise = 1.0 + pc.T * xi_effective + v_det;
        double snr = (noise > 0.0) ? signal / noise : 0.0;
        double Iab = SecurityAnalyzer::mutual_info(snr);

        m.I_AB = Iab;
        m.chi_BE = chi;
        m.K_asymptotic = std::max(0.0, m.I_AB - m.chi_BE);
        m.K_beta = std::max(0.0, pc.beta * m.I_AB - m.chi_BE);
    } else {
        m = sec.compute(pc.T, xi_effective, mod.modulation_variance());
    }

    auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // For logging, join attack names and params into single string
    std::string atk_name = attack_names_.empty() ? "none" : attack_names_.front();
    std::string atk_params = attack_params_.empty() ? "" : attack_params_.front();
    logger_->log(scenario, dist, pc.T, pc.xi, e, m, ms, atk_name, atk_params);

    std::cout << "[" << scenario << "] d=" << dist
              << " km | T̂=" << std::fixed << std::setprecision(3) << e.T_hat
              << " ξ̂=" << e.xi_hat
              << " | K_β=" << m.K_beta << " bit/sym | "
              << (m.secure() ? "SECURE" : "INSECURE") << '\n';
}

void ExperimentRunner::run() {
    for (double d : cfg_.distances_km) {
        run_one_point(d, cfg_.name);
    }
}

} // namespace cvqkd
