#include "experiment/experiment_runner.hpp"
#include "attacks/collective_attack.hpp"
#include "attacks/collective_model.hpp"
#include "attacks/intercept_resend_attack.hpp"
#include "attacks/intercept_resend_model.hpp"
#include "attacks/lo_manipulation_attack.hpp"
#include "attacks/saturation_attack.hpp"
#include "postprocessing/statistics.hpp"
#include "protocol/heterodyne_detector.hpp"
#include "protocol/qpsk_modulator.hpp"
#include "protocol/quantum_channel.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace cvqkd {
namespace {
using json = nlohmann::json;
constexpr double missing = std::numeric_limits<double>::quiet_NaN();

std::uint64_t mix_seed(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

const char* stage_name(AttackStage stage) {
    switch (stage) {
        case AttackStage::Source: return "source";
        case AttackStage::ChannelOutput: return "channel_output";
        case AttackStage::Measurement: return "measurement";
    }
    throw std::logic_error("invalid attack stage");
}

void apply_optical(const IAttack& attack, std::vector<Complex>& amplitudes) {
    Measurement m{VectorR(amplitudes.size()), VectorR(amplitudes.size())};
    for (Eigen::Index i = 0; i < m.X.size(); ++i) {
        m.X(i) = amplitudes[static_cast<std::size_t>(i)].real();
        m.P(i) = amplitudes[static_cast<std::size_t>(i)].imag();
    }
    attack.apply(m);
    for (Eigen::Index i = 0; i < m.X.size(); ++i)
        amplitudes[static_cast<std::size_t>(i)] = Complex(m.X(i), m.P(i));
}

json symbols_json(const SymbolStatistics& s) {
    return {{"counts", s.counts}, {"total", s.total}, {"observed", s.observed},
            {"erasures", s.total - s.observed}, {"I_discrete", s.mutual_information},
            {"SER", s.symbol_error_rate}, {"BER_gray", s.bit_error_rate}};
}

json metrics_json(const SecurityMetrics& m) {
    return {{"model", m.model}, {"T_used", m.T_used}, {"xi_used", m.xi_eff},
            {"I_AB", m.I_AB}, {"chi_BE", m.chi_BE}, {"K_beta", m.K_beta},
            {"Z_lower", m.correlation_bound}, {"n_B", m.bob_photon_number},
            {"projected_moments", m.projected_moments}, {"model_supported", m.model_supported},
            {"finite_key_certified", false}};
}

void attack_keys(const json& p, std::set<std::string> allowed) {
    allowed.insert({"name", "metadata"});
    for (const auto& item : p.items())
        if (allowed.count(item.key()) == 0 && item.key().find('_') != 0)
            throw std::invalid_argument("unknown attack parameter: " + item.key());
}

double saturation(const json& p, double fallback) {
    if (!p.contains("saturation_level")) return fallback;
    return p.at("saturation_level").is_null() ? std::numeric_limits<double>::infinity() :
                                               p.at("saturation_level").get<double>();
}

struct ConfiguredAttack {
    std::unique_ptr<IAttack> attack;
    std::unique_ptr<IAttackModel> prediction;
    json parameters;
    bool gaussian = true;
    bool physical = true;
};

std::vector<ConfiguredAttack> configure_attacks(const ExperimentConfig& cfg,
                                               const ProtocolConfig& pc) {
    std::vector<ConfiguredAttack> result;
    std::size_t ir_count = 0;
    for (std::size_t i = 0; i < cfg.attacks.size(); ++i) {
        const std::string& name = cfg.attacks[i];
        json p = i < cfg.attack_params_json.size() && !cfg.attack_params_json[i].empty() ?
                     json::parse(cfg.attack_params_json[i]) : json::object();
        if (!p.is_object()) throw std::invalid_argument("attack parameters must be an object");
        p["name"] = name;
        ConfiguredAttack entry;
        if (name == "none") {
            attack_keys(p, {});
            continue;
        }
        const auto seed = mix_seed(pc.seed ^ (0x61747461636b0000ULL + i));
        if (name == "intercept_resend") {
            attack_keys(p, {"eve_mode", "meas_eff", "resend_gain", "v_el", "fraction", "saturation_level"});
            if (++ir_count > 1) throw std::invalid_argument("only one IR Eve per experiment is supported");
            const std::string mode = p.value("eve_mode", std::string("physical"));
            if (mode != "physical" && mode != "ideal" && mode != "real" && mode != "oracle")
                throw std::invalid_argument("unknown Eve mode: " + mode);
            const double eta = mode == "real" ? pc.eta : mode == "physical" ? p.value("meas_eff", 0.8) : 1.0;
            const double v_el = mode == "real" ? pc.v_el : mode == "physical" ? p.value("v_el", 0.0) : 0.0;
            const double sat = mode == "real" ? pc.saturation_level :
                mode == "physical" ? saturation(p, std::numeric_limits<double>::infinity()) :
                                     std::numeric_limits<double>::infinity();
            if (mode != "physical" && (p.value("meas_eff", eta) != eta ||
                p.value("v_el", v_el) != v_el || saturation(p, sat) != sat))
                throw std::invalid_argument("Eve mode fixes her calibration; use physical to customize it");
            const double gain = p.value("resend_gain", 1.0), fraction = p.value("fraction", 1.0);
            const bool oracle = mode == "oracle";
            entry.attack = std::make_unique<InterceptResendAttack>(
                eta, gain, seed, v_el, fraction, sat, oracle ? EveMode::Oracle : EveMode::Physical);
            entry.prediction = std::make_unique<InterceptResendModel>(eta, gain, v_el, fraction, oracle);
            entry.gaussian = fraction == 0.0 || fraction == 1.0;
            entry.physical = !oracle || fraction == 0.0;
            p.update({{"eve_mode", mode}, {"meas_eff", eta}, {"v_el", v_el}, {"fraction", fraction},
                      {"resend_gain", gain}, {"saturation_level", sat}});
        } else if (name == "collective") {
            attack_keys(p, {"coupling", "excess_noise"});
            const double coupling = p.value("coupling", 0.3), excess = p.value("excess_noise", 0.05);
            entry.attack = std::make_unique<CollectiveAttack>(coupling, excess, seed);
            entry.prediction = std::make_unique<CollectiveModel>(coupling, excess);
            p.update({{"coupling", coupling}, {"excess_noise", excess}});
        } else if (name == "saturation") {
            attack_keys(p, {"saturation_level", "gain"});
            const double sat = saturation(p, pc.saturation_level), gain = p.value("gain", 1.0);
            if (std::isnan(sat) || sat <= 0.0 || !std::isfinite(gain) || gain <= 0.0)
                throw std::invalid_argument("invalid saturation attack parameters");
            entry.attack = std::make_unique<SaturationAttack>(sat, gain);
            entry.gaussian = entry.physical = false;
            p.update({{"saturation_level", sat}, {"gain", gain}});
        } else if (name == "lo" || name == "lo_manipulation" || name == "lo_manipulation_attack") {
            attack_keys(p, {"scale"});
            const double scale = p.value("scale", 1.0);
            if (!std::isfinite(scale) || scale <= 0.0)
                throw std::invalid_argument("LO scale must be finite and positive");
            entry.attack = std::make_unique<LOManipulationAttack>(scale);
            entry.gaussian = entry.physical = false;
            p["scale"] = scale;
        } else {
            throw std::invalid_argument("unsupported attack: " + name);
        }
        p["seed"] = seed;
        p["stage"] = stage_name(entry.attack->stage());
        entry.parameters = p;
        result.push_back(std::move(entry));
    }
    std::stable_sort(result.begin(), result.end(), [](const ConfiguredAttack& a, const ConfiguredAttack& b) {
        return a.attack->stage() < b.attack->stage();
    });
    return result;
}
} // namespace

ExperimentRunner::ExperimentRunner(ExperimentConfig cfg, std::shared_ptr<StatisticsLogger> logger)
    : cfg_(std::move(cfg)), logger_(std::move(logger)) {
    cfg_.validate();
    if (!logger_) throw std::invalid_argument("experiment logger is required");
    configure_attacks(cfg_, cfg_.base);
}

void ExperimentRunner::attach_attack(std::unique_ptr<IAttack> attack) {
    if (!attack) throw std::invalid_argument("cannot attach a null attack");
    attacks_.push_back(std::move(attack));
}

void ExperimentRunner::run() {
    logger_->write_header();
    const std::size_t points = std::max<std::size_t>(1, cfg_.distances_km.size());
    for (std::size_t point = 0; point < points; ++point) {
        for (std::size_t repeat = 0; repeat < cfg_.runs_per_point; ++repeat) {
            auto pc = cfg_.base;
            if (!cfg_.distances_km.empty()) {
                pc.distance_km = cfg_.distances_km[point];
                pc.recompute_T_from_distance();
            }
            pc.seed = mix_seed(cfg_.base.seed + point * cfg_.runs_per_point + repeat);
            run_one_point(pc, point, repeat);
        }
    }
}

void ExperimentRunner::run_one_point(ProtocolConfig pc, std::size_t point, std::size_t repeat) {
    const auto start = std::chrono::steady_clock::now();
    auto configured = configure_attacks(cfg_, pc);
    std::vector<const IAttack*> chain;
    const InterceptResendAttack* eve = nullptr;
    json attack_parameters = json::array();
    bool gaussian_residuals = attacks_.empty(), physical = attacks_.empty();
    bool prediction_supported = attacks_.empty();
    double predicted_T = pc.T, predicted_xi = pc.xi;
    std::string attack_name;
    for (const auto& entry : configured) {
        chain.push_back(entry.attack.get());
        if (const auto* ir = dynamic_cast<const InterceptResendAttack*>(entry.attack.get())) eve = ir;
        attack_parameters.push_back(entry.parameters);
        if (!attack_name.empty()) attack_name += "+";
        attack_name += entry.attack->name();
        gaussian_residuals = gaussian_residuals && entry.gaussian;
        physical = physical && entry.physical;
        if (entry.prediction && prediction_supported && predicted_T > 0.0 && predicted_T <= 1.0) {
            const auto prediction = entry.prediction->computeEffect(predicted_T, 2 * pc.alpha * pc.alpha, predicted_xi);
            predicted_T = prediction.transmission;
            predicted_xi = prediction.xi_total;
        } else if (!entry.prediction || predicted_T > 1.0) {
            prediction_supported = false;
        }
    }
    if (predicted_T == 0.0) predicted_xi = missing;
    for (const auto& attack : attacks_) {
        chain.push_back(attack.get());
        attack_parameters.push_back({{"name", attack->name()}, {"stage", stage_name(attack->stage())},
                                     {"external_configuration", true}});
    }
    if (attack_name.empty()) attack_name = "none";
    QPSKModulator mod(pc.alpha, pc.seed);
    QuantumChannel channel(pc);
    HeterodyneDetector detector(pc);
    const auto alice = mod.generate(pc.N);
    auto source = alice.amplitudes;
    for (const auto* attack : chain)
        if (attack->stage() == AttackStage::Source) apply_optical(*attack, source);
    auto optical_bob = channel.transmit(source);
    for (const auto* attack : chain)
        if (attack->stage() == AttackStage::ChannelOutput) apply_optical(*attack, optical_bob);
    auto bob = detector.measure(optical_bob);
    for (const auto* attack : chain)
        if (attack->stage() == AttackStage::Measurement) attack->apply(bob);

    const auto n = static_cast<Eigen::Index>(pc.N);
    VectorR x(2 * n), y(2 * n);
    for (Eigen::Index i = 0; i < n; ++i) {
        x(i) = alice.amplitudes[static_cast<std::size_t>(i)].real();
        x(n + i) = alice.amplitudes[static_cast<std::size_t>(i)].imag();
    }
    y.head(n) = bob.X;
    y.tail(n) = bob.P;
    const auto estimate = ParameterEstimator(pc, pc.confidence_level).estimate(x, y);
    const auto ab = symbol_statistics(alice.keys, bob);
    const double c2 = x.dot(y) / static_cast<double>(n);
    const double n_b = y.squaredNorm() / static_cast<double>(n) - 1.0;
    const auto eve_clipped = eve ? eve->clipped_components() : 0;
    const bool clipped = detector.clipped_components() != 0 || eve_clipped != 0;
    gaussian_residuals = gaussian_residuals && !clipped;
    physical = physical && !clipped;
    prediction_supported = prediction_supported && !clipped;
    const SecurityAnalyzer security(pc);
    SecurityMetrics metrics;
    if (physical) {
        metrics = security.compute_qpsk_moments(c2, n_b, pc.alpha, ab.mutual_information);
    } else {
        metrics.model = "unsupported_oracle_or_receiver_transformation";
        metrics.model_supported = false;
        metrics.I_AB = ab.mutual_information;
        metrics.chi_BE = metrics.K_asymptotic = metrics.K_beta = metrics.xi_eff = missing;
    }
    json details = {
        {"schema_version", 2}, {"point_index", point}, {"run_index", repeat},
        {"source_revision", source_revision()}, {"source_fingerprint", source_fingerprint()},
        {"base_seed", cfg_.base.seed}, {"seed", pc.seed}, {"N", pc.N},
        {"runs_per_point", cfg_.runs_per_point},
        {"protocol", {{"alpha", pc.alpha}, {"V_A", mod.quadrature_variance()}, {"T", pc.T},
                      {"xi", pc.xi}, {"eta", pc.eta}, {"v_el", pc.v_el}, {"beta", pc.beta},
                      {"saturation_level", pc.saturation_level}, {"confidence_level", pc.confidence_level}}},
        {"attacks", attack_parameters}, {"Alice_Bob", symbols_json(ab)},
        {"Bob_clipped_components", detector.clipped_components()}, {"Eve_clipped_components", eve_clipped},
        {"prediction", {{"supported", prediction_supported}, {"T", predicted_T}, {"xi", predicted_xi}}},
        {"estimate", {{"slope", estimate.slope}, {"slope_se", estimate.slope_se},
                       {"slope_lower", estimate.slope_lower}, {"slope_upper", estimate.slope_upper},
                       {"T_raw", estimate.T_hat}, {"xi_raw", estimate.xi_hat},
                       {"T_lower", estimate.T_lower}, {"T_upper", estimate.T_upper},
                       {"xi_lower", estimate.xi_lower}, {"xi_upper", estimate.xi_upper},
                       {"variance_lower", estimate.variance_lower}, {"variance_upper", estimate.variance_upper},
                       {"residual_variance", estimate.sigma2_residual}, {"identifiable", estimate.identifiable},
                       {"gaussian_interval_assumptions", gaussian_residuals}}},
        {"moments", {{"c2", c2}, {"n_B_raw", n_b},
                      {"residual_mean", (y - estimate.slope * x).mean()},
                      {"residual_XP", (bob.X - estimate.slope * x.head(n)).dot(
                           bob.P - estimate.slope * x.tail(n)) / static_cast<double>(n)}}},
        {"security", metrics_json(metrics)}
    };
    if (eve) details["Alice_Eve"] = symbols_json(symbol_statistics(alice.keys, eve->eve_measurement(), eve->intercepted()));
    if (estimate.identifiable && physical) {
        const double T_used = std::clamp(estimate.T_hat, 0.0, 1.0);
        const double xi_used = std::max(0.0, estimate.xi_hat);
        details["gaussian_reference"] = metrics_json(security.compute(T_used, xi_used, mod.quadrature_variance()));
        details["gaussian_reference"]["parameter_projection"] = T_used != estimate.T_hat || xi_used != estimate.xi_hat;
        if (gaussian_residuals) {
            const double information = qpsk_awgn_mutual_information(pc.alpha, std::sqrt(pc.eta * T_used),
                (1 + pc.v_el) / 2 + pc.eta * T_used * xi_used / 4);
            details["qpsk_awgn_reference"] = metrics_json(security.compute_qpsk(T_used, xi_used, pc.alpha, information));
        }
    }
    const double elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    logger_->log(cfg_.name, pc.distance_km, pc.T, pc.xi, estimate, metrics, elapsed,
                 attack_name, attack_parameters.dump(), details);
    std::cout << cfg_.name << " point=" << point << " run=" << repeat << " seed=" << pc.seed
              << " T_hat=" << estimate.T_hat << " xi_hat=" << estimate.xi_hat << '\n';
}

} // namespace cvqkd
