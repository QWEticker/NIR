#pragma once
#include "attacks/attack_model.hpp"
#include <Eigen/Dense>

namespace cvqkd {

class EntanglingClonerModel : public IAttackModel {
public:
    explicit EntanglingClonerModel(double xi_extra = 0.0);
    AttackResult computeEffect(double T, double V_A, double xi_in) const override;
    Eigen::Matrix<double, 6, 6> covariance(double T, double V_A, double xi_in) const;
    std::string name() const override { return "entangling_cloner"; }
private:
    double xi_extra_;
};

} // namespace cvqkd
