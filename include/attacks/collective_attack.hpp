#pragma once
#include "attacks/i_attack.hpp"
#include "core/rng.hpp"
#include <random>

namespace cvqkd {

/// Коллективная атака (Collective Attack).
/// Ева взаимодействует с каждым сигналом независимо, сохраняя квантовую память
/// для последующего совместного измерения. Моделируется добавлением избыточного шума
/// и частичной утечкой информации через параметр coupling_strength.
class CollectiveAttack final : public IAttack {
public:
    /// @param coupling_strength Сила взаимодействия с сигналом (0..1)
    /// @param excess_noise Дополнительный шум, вносимый атакой (в SNU)
    /// @param seed Seed для ГПСЧ
    explicit CollectiveAttack(double coupling_strength = 0.3,
                              double excess_noise = 0.05,
                              std::uint64_t seed = 54321);

    void        apply(Measurement& m) const override;
    std::string name() const override { return "collective"; }

private:
    double coupling_;
    double excess_noise_;
    mutable std::mt19937_64 rng_;
    mutable std::normal_distribution<double> noise_;
};

} // namespace cvqkd
