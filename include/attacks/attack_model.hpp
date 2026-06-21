#pragma once

#include <Eigen/Dense>
#include <string>

namespace cvqkd {

enum class AttackResultType { XiAdd, Covariance };

struct AttackResult {
    AttackResultType type;
    double xi_add = 0.0; // valid if type==XiAdd
    Eigen::Matrix4d modified_cm = Eigen::Matrix4d::Zero(); // valid if type==Covariance
};

// Abstract model: compute effect of attack given channel params.
// Implementations should compute either an added xi (SNU) or a modified 4x4 covariance matrix for B+E.
class IAttackModel {
public:
    virtual ~IAttackModel() = default;
    // T: channel transmissivity, V_A: modulation variance (Alice), xi_in: base channel excess noise
    virtual AttackResult computeEffect(double T, double V_A, double xi_in) const = 0;
    virtual std::string name() const = 0;
};

} // namespace cvqkd
