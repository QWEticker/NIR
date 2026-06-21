#pragma once
#include "attacks/attack_model.hpp"

namespace cvqkd {

class SaturationModel : public IAttackModel {
public:
    explicit SaturationModel(double sat = 10.0, double gain = 1.0);
    AttackResult computeEffect(double T, double V_A, double xi_in) const override;
    std::string name() const override { return "saturation"; }
private:
    double sat_;
    double gain_;
};

} // namespace cvqkd
