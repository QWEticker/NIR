#include "experiment/experiment_runner.hpp"
#include "postprocessing/parameter_estimator.hpp"
#include "postprocessing/security_analyzer.hpp"
#include "protocol/heterodyne_detector.hpp"
#include "protocol/qpsk_modulator.hpp"
#include "protocol/quantum_channel.hpp"
#include <chrono>
#include <iostream>

namespace cvqkd {

ExperimentRunner::ExperimentRunner(ExperimentConfig c,
                                   std::shared_ptr<StatisticsLogger> l)
    : cfg_(std::move(c)), logger_(std::move(l)) {}

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
    ParameterEstimator est(0.95);
    SecurityAnalyzer   sec(pc);

    auto t0 = std::chrono::steady_clock::now();

    auto alice = mod.generate(pc.N);
    auto bob_in = ch.transmit(alice.amplitudes);
    auto meas   = det.measure(bob_in);

    for (const auto& a : attacks_) a->apply(meas);

    // Формируем вектора для оценки: используем ОБЕ квадратуры (X и P)
    // xA: [Re(α₀), ..., Re(α_N), Im(α₀), ..., Im(α_N)]
    // yB: [meas.X, meas.P]
    VectorR xA(2 * pc.N), yB(2 * pc.N);
    for (std::size_t i = 0; i < pc.N; ++i) {
        xA(i)            = alice.amplitudes[i].real();
        xA(pc.N + i)     = alice.amplitudes[i].imag();
        yB(i)            = meas.X(static_cast<Eigen::Index>(i));
        yB(pc.N + i)     = meas.P(static_cast<Eigen::Index>(i));
    }

    auto e = est.estimate(xA, yB);
    // Ограничиваем оценку физически допустимыми пределами
    e.T_hat   = std::clamp(e.T_hat, 0.01, 1.0);
    e.xi_hat  = std::max(0.0, e.xi_hat);
    
    auto m = sec.compute(e.T_hat, e.xi_hat, mod.modulation_variance());

    auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    logger_->log(scenario, dist, pc.T, pc.xi, e, m, ms);

    std::cout << "[" << scenario << "] d=" << dist
              << " km | T̂=" << std::fixed << std::setprecision(3) << e.T_hat
              << " ξ̂=" << e.xi_hat
              << " | K_β=" << m.K_beta << " bit/sym | "
              << (m.secure() ? "SECURE" : "INSECURE") << '\n';
}

void ExperimentRunner::run() {
    if (cfg_.distances_km.empty())
        cfg_.distances_km = {cfg_.base.distance_km};

    for (double d : cfg_.distances_km) {
        for (std::size_t r = 0; r < cfg_.runs_per_point; ++r) {
            cfg_.base.seed += 1; // варьировать seed между прогонами
            run_one_point(d, cfg_.name);
        }
    }
}

} // namespace cvqkd