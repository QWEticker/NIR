#include "attacks/entangling_cloner_model.hpp"
#include "postprocessing/parameter_estimator.hpp"
#include "postprocessing/security_analyzer.hpp"
#include "postprocessing/statistics.hpp"
#include "json.hpp"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>

namespace {
using namespace cvqkd;
using json = nlohmann::json;

ProtocolConfig config(const json& input) {
    ProtocolConfig cfg;
    cfg.alpha = input.value("alpha", 0.5);
    cfg.T = input.value("T", 0.5);
    cfg.xi = input.value("xi", 0.01);
    cfg.eta = input.value("eta", 0.6);
    cfg.v_el = input.value("v_el", 0.05);
    cfg.beta = input.value("beta", 0.95);
    cfg.confidence_level = input.value("confidence_level", 0.95);
    cfg.validate();
    return cfg;
}

json metrics(const SecurityMetrics& m) {
    return {{"I_AB", m.I_AB}, {"chi_BE", m.chi_BE}, {"K_beta", m.K_beta},
            {"Z_lower", m.correlation_bound}, {"n_B", m.bob_photon_number}};
}

json estimates(const ChannelEstimate& e) {
    return {{"slope", e.slope}, {"slope_se", e.slope_se}, {"slope_lower", e.slope_lower},
            {"slope_upper", e.slope_upper}, {"T", e.T_hat}, {"xi", e.xi_hat},
            {"T_lower", e.T_lower}, {"T_upper", e.T_upper},
            {"xi_lower", e.xi_lower}, {"xi_upper", e.xi_upper},
            {"variance", e.sigma2_residual}, {"variance_lower", e.variance_lower},
            {"variance_upper", e.variance_upper}, {"identifiable", e.identifiable}};
}

json reference(const json& input) {
    const auto cfg = config(input);
    SecurityAnalyzer security(cfg);
    const double V_A = 2 * cfg.alpha * cfg.alpha;
    const auto source = SecurityAnalyzer::qpsk_source_moments(cfg.alpha);
    const double info = qpsk_awgn_mutual_information(cfg.alpha, std::sqrt(cfg.eta * cfg.T),
        (1 + cfg.v_el) / 2 + cfg.eta * cfg.T * cfg.xi / 4);
    json result = {{"input", input}, {"gaussian", metrics(security.compute(cfg.T, cfg.xi, V_A))},
                    {"qpsk", metrics(security.compute_qpsk(cfg.T, cfg.xi, cfg.alpha, info))},
                    {"source", {{"eigenvalues", source.eigenvalues}, {"trace", source.trace}, {"w", source.w}}}};
    if (cfg.T < 1.0 || cfg.xi == 0.0) {
        const auto cm = EntanglingClonerModel().covariance(cfg.T, V_A, cfg.xi);
        auto values = json::array();
        for (Eigen::Index i = 0; i < 6; ++i) {
            auto row = json::array();
            for (Eigen::Index j = 0; j < 6; ++j) row.push_back(cm(i, j));
            values.push_back(row);
        }
        result["dilation"] = values;
    }
    return result;
}

json coverage(const json& input) {
    const auto cfg = config(input);
    const auto repeats = input.at("repeats").get<std::size_t>();
    const auto count = input.at("samples").get<Eigen::Index>();
    const auto seed = input.at("seed").get<std::uint64_t>();
    if (repeats == 0 || repeats > 100000 || count < 3 || count > 10000000)
        throw std::invalid_argument("invalid coverage experiment dimensions");
    const double gain = std::sqrt(cfg.eta * cfg.T);
    const double sigma = std::sqrt((1 + cfg.v_el) / 2 + cfg.eta * cfg.T * cfg.xi / 4);
    const ParameterEstimator estimator(cfg, cfg.confidence_level);
    auto runs = json::array();
    for (std::size_t run = 0; run < repeats; ++run) {
        const auto run_seed = seed + run;
        std::mt19937_64 rng(run_seed);
        std::normal_distribution<double> noise(0.0, sigma);
        VectorR x(count), y(count);
        for (Eigen::Index i = 0; i < count; ++i) {
            x(i) = i % 2 == 0 ? 0.0 : i % 4 == 1 ? cfg.alpha : -cfg.alpha;
            y(i) = gain * x(i) + noise(rng);
        }
        auto result = estimates(estimator.estimate(x, y));
        result["seed"] = run_seed;
        runs.push_back(result);
    }
    return runs;
}
} // namespace

int main() {
    try {
        json input;
        std::cin >> input;
        json output = {{"source_revision", source_revision()}, {"source_fingerprint", source_fingerprint()}};
        const std::string mode = input.at("mode");
        auto values = json::array();
        if (mode == "reference") {
            for (const auto& item : input.at("cases")) values.push_back(reference(item));
        } else if (mode == "quantiles") {
            for (const auto& item : input.at("cases")) {
                const double p = item.at("p"), df = item.at("df");
                values.push_back({{"student", student_quantile(p, df)},
                                  {"chi_square", chi_square_quantile(p, df)}});
            }
        } else if (mode == "estimate") {
            for (const auto& item : input.at("cases")) {
                const auto x = item.at("x").get<std::vector<double>>();
                const auto y = item.at("y").get<std::vector<double>>();
                const auto cfg = config(item);
                values.push_back(estimates(ParameterEstimator(cfg, cfg.confidence_level).estimate(
                    VectorR::Map(x.data(), static_cast<Eigen::Index>(x.size())),
                    VectorR::Map(y.data(), static_cast<Eigen::Index>(y.size())))));
            }
        } else if (mode == "coverage") {
            values = coverage(input);
        } else {
            throw std::invalid_argument("unsupported probe mode: " + mode);
        }
        output["values"] = values;
        std::cout << output.dump() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "validation probe: " << error.what() << '\n';
        return 1;
    }
}
