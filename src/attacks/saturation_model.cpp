#include "attacks/saturation_model.hpp"
#include <algorithm>
#include <cmath>

namespace cvqkd {

SaturationModel::SaturationModel(double sat, double gain) : sat_(sat), gain_(gain) {}

// Saturation model: approximate clipping at detector by comparing mode variance to sat^2.
// Returns an added excess noise xi_add = max(0, (V - sat^2)/V) where V = V_A + 1.
AttackResult SaturationModel::computeEffect(double /*T*/, double V_A, double /*xi_in*/) const {
    AttackResult r;
    double V = V_A + 1.0;
    double sat2 = (sat_ * sat_);
    double xi_add = 0.0;
    if (sat2 < V) xi_add = (V - sat2) / std::max(1e-12, V);
    // incorporate gain tweak (if gain != 1, effective clipping changes)
    if (gain_ != 1.0) xi_add *= std::abs(gain_);
    r.type = AttackResultType::XiAdd;
    r.xi_add = std::max(0.0, xi_add);
    return r;
}

} // namespace cvqkd
