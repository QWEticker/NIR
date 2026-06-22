#include "attacks/lo_manipulation_model.hpp"
#include <algorithm>
#include <cmath>

namespace cvqkd {

LOManipulationModel::LOManipulationModel(double scale) : scale_(scale) {}

// LO manipulation model: scaling LO amplitude by 'scale' changes effective shot-noise calibration.
// We model this as an added excess noise xi_add = (1/s^2 - 1) (shot-noise inflation), clipped to >=0.
AttackResult LOManipulationModel::computeEffect(double /*T*/, double /*V_A*/, double /*xi_in*/) const {
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

} // namespace cvqkd
