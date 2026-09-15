#include "attacks/intercept_resend_model.hpp"
#include <cmath>
#include <stdexcept>

namespace cvqkd {

InterceptResendModel::InterceptResendModel(double efficiency, double gain, double noise,
                                         double fraction, bool oracle)
    : meas_eff_(efficiency), resend_gain_(gain), v_el_(noise), fraction_(fraction),
      oracle_(oracle) {
    if (!std::isfinite(efficiency) || efficiency <= 0.0 || efficiency > 1.0 ||
        !std::isfinite(gain) || gain < 0.0 ||
        !std::isfinite(noise) || noise < 0.0 ||
        !std::isfinite(fraction) || fraction < 0.0 || fraction > 1.0)
        throw std::invalid_argument("invalid intercept-resend prediction parameters");
}

AttackResult InterceptResendModel::computeEffect(double T, double V_A, double xi) const {
    if (!std::isfinite(T) || T < 0.0 || T > 1.0 ||
        !std::isfinite(V_A) || V_A < 0.0 || !std::isfinite(xi) || xi < 0.0)
        throw std::invalid_argument("invalid channel prediction input");
    const double mean_gain = 1.0 + fraction_ * (resend_gain_ - 1.0);
    const double gain2 = mean_gain * mean_gain;
    const double eve_noise = oracle_ ? 0.0 : 2.0 * (1.0 + v_el_) / meas_eff_;
    const double gain_variance = fraction_ * (1.0 - fraction_) *
                                (resend_gain_ - 1.0) * (resend_gain_ - 1.0);
    AttackResult result;
    result.type = AttackResultType::XiAdd;
    result.transmission = T * gain2;
    if (T > 0.0 && gain2 > 0.0) {
        result.xi_total = (xi + fraction_ * resend_gain_ * resend_gain_ * eve_noise +
                           V_A * gain_variance) / gain2;
        result.xi_add = result.xi_total - xi;
    }
    return result;
}

} // namespace cvqkd
