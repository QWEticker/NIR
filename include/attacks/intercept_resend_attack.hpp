#pragma once
#include "attacks/i_attack.hpp"
#include "core/rng.hpp"
#include <random>

namespace cvqkd {

/// Атака перехвата-и-пересылки (Intercept-Resend Attack).
/// Ева измеряет сигнал Алисы, получает результат и посылает Бобу
/// новое когерентное состояние согласно своему измерению.
class InterceptResendAttack final : public IAttack {
public:
    /// @param measurement_efficiency Эффективность измерения Евы (0..1)
    /// @param resending_gain Коэффициент усиления при пересылке (обычно 1.0)
    /// @param seed Seed для ГПСЧ Евы
    explicit InterceptResendAttack(double measurement_efficiency = 0.8,
                                   double resending_gain = 1.0,
                                   std::uint64_t seed = 12345);

    void        apply(Measurement& m) const override;
    std::string name() const override { return "intercept_resend"; }

private:
    double meas_eff_;
    double resend_gain_;
    mutable std::mt19937_64 rng_;
    mutable std::normal_distribution<double> noise_;
};

} // namespace cvqkd
