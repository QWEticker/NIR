#include "attacks/attack_model.hpp"

namespace cvqkd {

// Intercept-Resend model: approximate the effect as an added excess noise xi
class InterceptResendModel : public IAttackModel {
public:
    explicit InterceptResendModel(double measurement_eff = 0.8, double resend_gain = 1.0)
        : meas_eff_(measurement_eff), resend_gain_(resend_gain) {}

    AttackResult computeEffect(double /*T*/, double /*V_A*/, double /*xi_in*/) const override {
        AttackResult r;
        // Approximate added noise (SNU) due to imperfect measurement-and-resend.
        // A reasonable first-order model: xi_add = (1 - meas_eff)/meas_eff.
        double xi_add = 0.0;
        if (meas_eff_ > 1e-12) xi_add = (1.0 - meas_eff_) / std::max(1e-12, meas_eff_);
        // Scale with resend_gain deviations (if resend_gain != 1, additional noise)
        if (resend_gain_ != 1.0) xi_add *= std::abs(resend_gain_ - 1.0) + 1.0;

        r.type = AttackResultType::XiAdd;
        r.xi_add = xi_add;
        return r;
    }

    std::string name() const override { return "intercept_resend"; }

private:
    double meas_eff_;
    double resend_gain_;
};

} // namespace cvqkd
