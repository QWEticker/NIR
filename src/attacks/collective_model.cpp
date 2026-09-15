#include "attacks/collective_model.hpp"
#include <cmath>
#include <stdexcept>

namespace cvqkd {

CollectiveModel::CollectiveModel(double coupling, double excess)
    : coupling_(coupling), excess_noise_(excess) {
    if (!std::isfinite(coupling) || coupling < 0.0 || coupling > 1.0 ||
        !std::isfinite(excess) || excess < 0.0)
        throw std::invalid_argument("invalid collective channel");
}

AttackResult CollectiveModel::computeEffect(double T, double V_A, double xi) const {
    if (!std::isfinite(T) || T < 0.0 || T > 1.0 ||
        !std::isfinite(V_A) || V_A < 0.0 || !std::isfinite(xi) || xi < 0.0)
        throw std::invalid_argument("invalid channel prediction input");
    AttackResult result;
    result.type = AttackResultType::XiAdd;
    result.transmission = T * (1.0 - coupling_ * coupling_);
    if (result.transmission > 0.0) {
        result.xi_add = excess_noise_ / T;
        result.xi_total = xi + result.xi_add;
    }
    return result;
}

} // namespace cvqkd
