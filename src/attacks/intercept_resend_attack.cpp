#include "attacks/intercept_resend_attack.hpp"
#include <cmath>

namespace cvqkd {

InterceptResendAttack::InterceptResendAttack(double measurement_efficiency,
                                             double resending_gain,
                                             std::uint64_t seed)
    : meas_eff_(measurement_efficiency)
    , resend_gain_(resending_gain)
    , rng_(seed)
    , noise_(0.0, 1.0) {}

void InterceptResendAttack::apply(Measurement& m) const {
    // Моделирование атаки перехвата-и-пересылки:
    // 1. Ева измеряет сигнал с эффективностью meas_eff_
    // 2. Добавляется шум квантового измерения (вакуумный шум)
    // 3. Ева посылает Бобу новое состояние с амплитудой, пропорциональной её результату
    
    const double vacuum_noise = 1.0; // SNU
    const double shot_noise_factor = std::sqrt((1.0 - meas_eff_) / meas_eff_);
    
    for (Eigen::Index i = 0; i < m.X.size(); ++i) {
        // Ева получает зашумлённую версию квадрaтур
        const double eve_X = m.X(i) * std::sqrt(meas_eff_) + 
                             noise_(rng_) * shot_noise_factor * vacuum_noise;
        const double eve_P = m.P(i) * std::sqrt(meas_eff_) + 
                             noise_(rng_) * shot_noise_factor * vacuum_noise;
        
        // Ева посылает Бобу своё измеренное значение (с возможным усилением)
        m.X(i) = resend_gain_ * eve_X;
        m.P(i) = resend_gain_ * eve_P;
    }
}

} // namespace cvqkd
