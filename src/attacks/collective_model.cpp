#include "attacks/collective_model.hpp"
#include <algorithm>

namespace cvqkd {

CollectiveModel::CollectiveModel(double coupling, double excess_noise)
    : coupling_(coupling), excess_noise_(excess_noise) {}

// Коллективная атака с делителем луча связи 'coupling' вносит избыточный шум
// excess_noise плюс вклад от отвода части сигнала Еве (~coupling^2).
AttackResult CollectiveModel::computeEffect(double /*T*/, double /*V_A*/, double /*xi_in*/) const {
    AttackResult r;
    r.type   = AttackResultType::XiAdd;
    r.xi_add = std::max(0.0, excess_noise_ + coupling_ * coupling_);
    return r;
}

} // namespace cvqkd
