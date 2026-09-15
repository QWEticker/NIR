#include "postprocessing/parameter_estimator.hpp"
#include "postprocessing/statistics.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace cvqkd {

ParameterEstimator::ParameterEstimator(const ProtocolConfig& cfg, double confidence)
    : cfg_(cfg), conf_(confidence) {
    if (!std::isfinite(cfg.eta) || cfg.eta <= 0.0 || cfg.eta > 1.0 ||
        !std::isfinite(cfg.v_el) || cfg.v_el < 0.0 ||
        !std::isfinite(confidence) || confidence <= 0.0 || confidence >= 1.0)
        throw std::invalid_argument("invalid estimator calibration or confidence level");
}

ChannelEstimate ParameterEstimator::estimate(const VectorR& x, const VectorR& y) const {
    if (x.size() != y.size() || x.size() < 3 || !x.allFinite() || !y.allFinite())
        throw std::invalid_argument("estimation requires matching finite vectors, n >= 3");
    const double xx = x.squaredNorm();
    if (!std::isfinite(xx) || xx <= 0.0)
        throw std::invalid_argument("Alice's signal must have finite nonzero energy");
    const double df = static_cast<double>(x.size() - 1);
    ChannelEstimate e{};
    e.n_samples = static_cast<std::size_t>(x.size());
    e.slope = x.dot(y) / xx;
    const double sse = (y - e.slope * x).squaredNorm();
    e.sigma2_residual = sse / df;
    if (!std::isfinite(e.slope) || !std::isfinite(e.sigma2_residual))
        throw std::overflow_error("regression overflow");
    e.slope_se = std::sqrt(e.sigma2_residual / xx);
    const double half = student_quantile((1.0 + conf_) / 2.0, df) * e.slope_se;
    e.slope_lower = e.slope - half;
    e.slope_upper = e.slope + half;
    e.T_hat = e.slope * e.slope / cfg_.eta;
    const double min_square = e.slope_lower <= 0.0 && e.slope_upper >= 0.0 ? 0.0 :
        std::min(e.slope_lower * e.slope_lower, e.slope_upper * e.slope_upper);
    e.T_lower = min_square / cfg_.eta;
    e.T_upper = std::max(e.slope_lower * e.slope_lower,
                         e.slope_upper * e.slope_upper) / cfg_.eta;
    e.T_ci_half = std::max(e.T_hat - e.T_lower, e.T_upper - e.T_hat);
    const double detector_variance = (1.0 + cfg_.v_el) / 2.0;
    e.xi_hat = e.slope == 0.0 ? std::numeric_limits<double>::quiet_NaN() :
        4.0 * (e.sigma2_residual - detector_variance) / (e.slope * e.slope);

    // Bonferroni rectangle for slope and variance under independent Gaussian noise.
    const double tail = (1.0 - conf_) / 4.0;
    const double joint_half = student_quantile(1.0 - tail, df) * e.slope_se;
    const double slope_lo = e.slope - joint_half, slope_hi = e.slope + joint_half;
    e.variance_lower = sse / chi_square_quantile(1.0 - tail, df);
    e.variance_upper = sse / chi_square_quantile(tail, df);
    e.identifiable = slope_lo > 0.0;
    if (slope_lo <= 0.0 && slope_hi >= 0.0) {
        e.xi_lower = -std::numeric_limits<double>::infinity();
        e.xi_upper = std::numeric_limits<double>::infinity();
    } else {
        const std::array<double, 4> bounds{
            4.0 * (e.variance_lower - detector_variance) / (slope_lo * slope_lo),
            4.0 * (e.variance_lower - detector_variance) / (slope_hi * slope_hi),
            4.0 * (e.variance_upper - detector_variance) / (slope_lo * slope_lo),
            4.0 * (e.variance_upper - detector_variance) / (slope_hi * slope_hi)};
        const auto range = std::minmax_element(bounds.begin(), bounds.end());
        e.xi_lower = *range.first;
        e.xi_upper = *range.second;
    }
    e.xi_ci_half = std::max(e.xi_hat - e.xi_lower, e.xi_upper - e.xi_hat);
    return e;
}

} // namespace cvqkd
