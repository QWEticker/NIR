#include "attacks/collective_attack.hpp"
#include <cmath>
#include <stdexcept>

namespace cvqkd {

CollectiveAttack::CollectiveAttack(double coupling, double excess, std::uint64_t seed)
    : coupling_(coupling), excess_noise_(excess), rng_(seed), noise_(0.0, 1.0) {
    if (!std::isfinite(coupling) || coupling < 0.0 || coupling > 1.0 ||
        !std::isfinite(excess) || excess < 0.0)
        throw std::invalid_argument("collective requires coupling in [0,1], excess_noise >= 0");
}

void CollectiveAttack::apply(Measurement& m) const {
    if (m.X.size() != m.P.size() || !m.X.allFinite() || !m.P.allFinite())
        throw std::invalid_argument("invalid optical amplitude components");
    const double t = 1.0 - coupling_ * coupling_;
    const double scale = std::sqrt(t);
    const double sigma = std::sqrt(t * excess_noise_ / 4.0);
    for (Eigen::Index i = 0; i < m.X.size(); ++i) {
        m.X(i) = scale * m.X(i) + sigma * noise_(rng_);
        m.P(i) = scale * m.P(i) + sigma * noise_(rng_);
    }
}

} // namespace cvqkd
