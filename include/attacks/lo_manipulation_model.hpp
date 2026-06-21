#pragma once
#include "attacks/attack_model.hpp"

namespace cvqkd {

class LOManipulationModel : public IAttackModel {
public:
    explicit LOManipulationModel(double scale = 1.0);
    AttackResult computeEffect(double T, double V_A, double xi_in) const override;
    std::string name() const override { return "lo_manipulation"; }
private:
    double scale_;
};

} // namespace cvqkd
