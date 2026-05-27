#include "protocol/heterodyne_detector.hpp"
#include <cmath>

namespace cvqkd {

HeterodyneDetector::HeterodyneDetector(const ProtocolConfig& cfg)
    : cfg_(cfg), rng_(cfg.seed ^ 0xDEADBEEFull) {}

double HeterodyneDetector::detector_noise_variance() const noexcept {
    // 1 (вакуум гетеродина) + v_el, делённое на η.
    return (1.0 + cfg_.v_el) / cfg_.eta;
}

Measurement HeterodyneDetector::measure(const std::vector<Complex>& in) {
    const std::size_t N = in.size();
    Measurement m;
    m.X.resize(static_cast<Eigen::Index>(N));
    m.P.resize(static_cast<Eigen::Index>(N));

    const double sqrt_eta = std::sqrt(cfg_.eta);
    const double sigma = std::sqrt((1.0 + cfg_.v_el) / cfg_.eta);

    for (std::size_t i = 0; i < N; ++i) {
        m.X(static_cast<Eigen::Index>(i)) =
            sqrt_eta * in[i].real() + rng_.normal(0.0, sigma);
        m.P(static_cast<Eigen::Index>(i)) =
            sqrt_eta * in[i].imag() + rng_.normal(0.0, sigma);
    }
    return m;
}

} // namespace cvqkd