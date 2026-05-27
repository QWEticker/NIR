#pragma once
#include "attacks/i_attack.hpp"

namespace cvqkd {

/// Клиппинг измерений детектора на уровне saturation_level.
class SaturationAttack final : public IAttack {
public:
    SaturationAttack(double saturation_level, double gain = 1.0)
        : sat_(saturation_level), gain_(gain) {}

    void        apply(Measurement& m) const override;
    std::string name() const override { return "saturation"; }

private:
    double sat_, gain_;
};

} // namespace cvqkd