#include "attacks/attack_model.hpp"
#include <cmath>

namespace cvqkd {

// Entangling cloner model: returns a modified 4x4 covariance matrix (B+E)
class EntanglingClonerModel : public IAttackModel {
public:
    explicit EntanglingClonerModel(double xi_extra = 0.0) : xi_extra_(xi_extra) {}

    AttackResult computeEffect(double T, double V_A, double xi_in) const override {
        AttackResult r;
        // total xi experienced by Bob (input referred)
        double xi_total = xi_in + xi_extra_;
        // Build CM following standard formula (same as HolevoCalculator::buildEntanglingClonerCM)
        double V = V_A + 1.0;
        double W = 1.0;
        if (T < 1.0 - 1e-12) {
            W = 1.0 + (T * xi_total) / (1.0 - T);
        } else {
            W = 1.0 + xi_total;
        }

        double b = T * V + (1.0 - T) * W;
        double e = (1.0 - T) * V + T * W;
        double c = std::sqrt(std::max(0.0, T * (1.0 - T))) * (V - W);

        Eigen::Matrix4d Vbe; Vbe.setZero();
        Vbe(0,0) = b; Vbe(1,1) = b;
        Vbe(2,2) = e; Vbe(3,3) = e;
        Vbe(0,2) = c; Vbe(2,0) = c;
        Vbe(1,3) = -c; Vbe(3,1) = -c;

        r.type = AttackResultType::Covariance;
        r.modified_cm = Vbe;
        return r;
    }

    std::string name() const override { return "entangling_cloner"; }

private:
    double xi_extra_;
};

} // namespace cvqkd
