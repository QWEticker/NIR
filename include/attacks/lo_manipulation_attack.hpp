#pragma once
#include "attacks/i_attack.hpp"

namespace cvqkd {

/// Масштабирование локального осциллятора (занижение/завышение амплитуды LO).
class LOManipulationAttack final : public IAttack {
public:
    explicit LOManipulationAttack(double lo_scale) : scale_(lo_scale) {}

    void        apply(Measurement& m) const override;
    std::string name() const override { return "lo_manipulation"; }

private:
    double scale_;
};

} // namespace cvqkd