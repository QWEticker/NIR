#include "attacks/intercept_resend_attack.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cvqkd {

InterceptResendAttack::InterceptResendAttack(double efficiency, double gain,
                                           std::uint64_t seed, double electronic_noise,
                                           double fraction, double saturation, EveMode mode)
    : meas_eff_(efficiency), resend_gain_(gain), v_el_(electronic_noise),
      fraction_(fraction), saturation_(saturation), mode_(mode),
      rng_(seed), noise_(0.0, 1.0) {
    if (!std::isfinite(efficiency) || efficiency <= 0.0 || efficiency > 1.0 ||
        !std::isfinite(gain) || gain < 0.0 ||
        !std::isfinite(electronic_noise) || electronic_noise < 0.0 ||
        !std::isfinite(fraction) || fraction < 0.0 || fraction > 1.0 ||
        std::isnan(saturation) || saturation <= 0.0)
        throw std::invalid_argument("invalid intercept-resend parameters");
}

void InterceptResendAttack::apply(Measurement& m) const {
    if (m.X.size() != m.P.size() || !m.X.allFinite() || !m.P.allFinite())
        throw std::invalid_argument("invalid optical amplitude components");
    eve_.X = VectorR::Zero(m.X.size());
    eve_.P = VectorR::Zero(m.P.size());
    intercepted_.assign(m.size(), 0);
    clipped_ = 0;
    std::bernoulli_distribution select(fraction_);
    const double gain = std::sqrt(meas_eff_);
    const double sigma = std::sqrt((1.0 + v_el_) / 2.0);
    for (Eigen::Index i = 0; i < m.X.size(); ++i) {
        if (fraction_ == 0.0 || (fraction_ < 1.0 && !select(rng_))) continue;
        intercepted_[static_cast<std::size_t>(i)] = 1;
        double x = m.X(i), p = m.P(i);
        if (mode_ == EveMode::Physical) {
            x = gain * x + sigma * noise_(rng_);
            p = gain * p + sigma * noise_(rng_);
            clipped_ += static_cast<std::size_t>(std::abs(x) > saturation_);
            clipped_ += static_cast<std::size_t>(std::abs(p) > saturation_);
            x = std::clamp(x, -saturation_, saturation_) / gain;
            p = std::clamp(p, -saturation_, saturation_) / gain;
        }
        eve_.X(i) = x;
        eve_.P(i) = p;
        m.X(i) = resend_gain_ * x;
        m.P(i) = resend_gain_ * p;
    }
}

} // namespace cvqkd
