#pragma once
#include "attacks/attack_model.hpp"

namespace cvqkd {

class InterceptResendModel : public IAttackModel {
public:
    explicit InterceptResendModel(double measurement_eff = 0.8, double resend_gain = 1.0,
                                 double electronic_noise = 0.0, double fraction = 1.0,
                                 bool oracle = false);
    AttackResult computeEffect(double T, double V_A, double xi_in) const override;
    std::string name() const override { return "intercept_resend"; }
private:
    double meas_eff_;
    double resend_gain_;
    double v_el_;
    double fraction_;
    bool oracle_;
};

} // namespace cvqkd
