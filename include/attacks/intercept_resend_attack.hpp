#pragma once
#include "attacks/i_attack.hpp"
#include "core/rng.hpp"
#include <random>
#include <limits>

namespace cvqkd {

enum class EveMode { Physical, Oracle };

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
                                   std::uint64_t seed = 12345,
                                   double electronic_noise = 0.0,
                                   double fraction = 1.0,
                                   double saturation = std::numeric_limits<double>::infinity(),
                                   EveMode mode = EveMode::Physical);

    void        apply(Measurement& m) const override;
    std::string name() const override { return "intercept_resend"; }
    AttackStage stage() const noexcept override { return AttackStage::Source; }
    const Measurement& eve_measurement() const noexcept { return eve_; }
    const std::vector<std::uint8_t>& intercepted() const noexcept { return intercepted_; }
    std::size_t clipped_components() const noexcept { return clipped_; }

private:
    double meas_eff_;
    double resend_gain_;
    double v_el_;
    double fraction_;
    double saturation_;
    EveMode mode_;
    mutable Measurement eve_;
    mutable std::vector<std::uint8_t> intercepted_;
    mutable std::size_t clipped_ = 0;
    mutable std::mt19937_64 rng_;
    mutable std::normal_distribution<double> noise_;
};

} // namespace cvqkd