#include "protocol/heterodyne_detector.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace cvqkd {

HeterodyneDetector::HeterodyneDetector(const ProtocolConfig& cfg)
    : cfg_(cfg), rng_(cfg.seed ^ 0xDEADBEEFull) {
    if (!std::isfinite(cfg.eta) || cfg.eta <= 0.0 || cfg.eta > 1.0 ||
        !std::isfinite(cfg.v_el) || cfg.v_el < 0.0 ||
        std::isnan(cfg.saturation_level) || cfg.saturation_level <= 0.0)
        throw std::invalid_argument("invalid heterodyne receiver parameters");
}

double HeterodyneDetector::detector_noise_variance() const noexcept {
    return (1.0 + cfg_.v_el) / 2.0;
}

Measurement HeterodyneDetector::measure(const std::vector<Complex>& in) {
    const std::size_t N = in.size();
    Measurement m;
    m.X.resize(static_cast<Eigen::Index>(N));
    m.P.resize(static_cast<Eigen::Index>(N));

    const double sqrt_eta = std::sqrt(cfg_.eta);
    const double sigma = std::sqrt(detector_noise_variance());
    clipped_ = 0;

    for (std::size_t i = 0; i < N; ++i) {
        if (!std::isfinite(in[i].real()) || !std::isfinite(in[i].imag()))
            throw std::invalid_argument("receiver input must be finite");
        m.X(static_cast<Eigen::Index>(i)) =
            sqrt_eta * in[i].real() + rng_.normal(0.0, sigma);
        m.P(static_cast<Eigen::Index>(i)) =
            sqrt_eta * in[i].imag() + rng_.normal(0.0, sigma);
        for (VectorR* component : {&m.X, &m.P}) {
            double& value = (*component)(static_cast<Eigen::Index>(i));
            if (std::abs(value) > cfg_.saturation_level) ++clipped_;
            value = std::clamp(value, -cfg_.saturation_level, cfg_.saturation_level);
        }
    }
    return m;
}

} // namespace cvqkd