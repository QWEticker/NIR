#include "attacks/entangling_cloner_model.hpp"
#include <cmath>
#include <stdexcept>

namespace cvqkd {

EntanglingClonerModel::EntanglingClonerModel(double extra) : xi_extra_(extra) {
    if (!std::isfinite(extra) || extra < 0.0)
        throw std::invalid_argument("entangling-cloner excess noise must be nonnegative");
}

Eigen::Matrix<double, 6, 6> EntanglingClonerModel::covariance(double T, double V_A,
                                                           double xi) const {
    const double excess = xi + xi_extra_;
    if (!std::isfinite(T) || T < 0.0 || T > 1.0 ||
        !std::isfinite(V_A) || V_A < 0.0 || !std::isfinite(xi) || xi < 0.0 ||
        !std::isfinite(excess) || (T == 1.0 && excess > 0.0))
        throw std::invalid_argument("finite thermal dilation requires T < 1 for nonzero noise");
    const double V = V_A + 1.0;
    const double W = T == 1.0 ? 1.0 : 1.0 + T * excess / (1.0 - T);
    const double thermal_correlation = std::sqrt((W - 1.0) * (W + 1.0));
    Eigen::Matrix<double, 6, 6> cm = Eigen::Matrix<double, 6, 6>::Zero();
    cm.block<2, 2>(0, 0) = (T * V + (1.0 - T) * W) * Eigen::Matrix2d::Identity();
    cm.block<2, 2>(2, 2) = ((1.0 - T) * V + T * W) * Eigen::Matrix2d::Identity();
    cm.block<2, 2>(4, 4) = W * Eigen::Matrix2d::Identity();
    Eigen::Matrix2d Z = Eigen::Matrix2d::Identity();
    Z(1, 1) = -1.0;
    cm.block<2, 2>(0, 2) = std::sqrt(T * (1.0 - T)) * (W - V) * Eigen::Matrix2d::Identity();
    cm.block<2, 2>(0, 4) = std::sqrt(1.0 - T) * thermal_correlation * Z;
    cm.block<2, 2>(2, 4) = std::sqrt(T) * thermal_correlation * Z;
    cm.block<2, 2>(2, 0) = cm.block<2, 2>(0, 2).transpose();
    cm.block<2, 2>(4, 0) = cm.block<2, 2>(0, 4).transpose();
    cm.block<2, 2>(4, 2) = cm.block<2, 2>(2, 4).transpose();
    return cm;
}

AttackResult EntanglingClonerModel::computeEffect(double T, double V_A, double xi) const {
    AttackResult result;
    result.type = AttackResultType::Covariance;
    result.dilation = covariance(T, V_A, xi);
    result.modified_cm = result.dilation.topLeftCorner<4, 4>();
    result.transmission = T;
    result.xi_total = xi + xi_extra_;
    return result;
}

} // namespace cvqkd
