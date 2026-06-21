#include "attacks/attack_model.hpp"
#include <cmath>

namespace cvqkd {

// LO manipulation model: scaling LO amplitude by 'scale' changes effective shot-noise calibration.
// We model this as an added excess noise xi_add = (1/s^2 - 1) (shot-noise inflation), clipped to >=0.
class LOManipulationModel : public IAttackModel {
public:
    explicit LOManipulationModel(double scale = 1.0) : scale_(scale) {}

    AttackResult computeEffect(double /*T*/, double /*V_A*/, double /*xi_in*/) const override {
        AttackResult r;
        double s = scale_;
        double xi_add = 0.0;
        if (s > 1e-12) {
            xi_add = std::max(0.0, (1.0 / (s * s)) - 1.0);
        }
        r.type = AttackResultType::XiAdd;
        r.xi_add = xi_add;
        return r;
    }

    std::string name() const override { return "lo_manipulation"; }

private:
    double scale_;
};

} // namespace cvqkd
