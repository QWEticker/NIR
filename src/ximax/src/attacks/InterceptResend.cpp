#pragma once

#include "../include/AttackBase.h"

namespace nir {

class InterceptResend : public AttackBase {
public:
    // f: fraction of pulses intercepted (0..1)
    // xi_per_event: effective excess noise introduced per intercepted pulse (SNU)
    InterceptResend(double f = 0.0, double xi_per_event = 1.0) : f_(f), xi_per_event_(xi_per_event) {}

    double inducedExcessNoise() const override {
        // Approximate added excess noise proportional to intercepted fraction
        return std::min(1.0, f_ * xi_per_event_);
    }

    void modifyCovariance(Eigen::Matrix4d &V_BE) const override {
        // Increase Bob's diagonal variance proportionally to the intercepted fraction
        double add = inducedExcessNoise();
        V_BE(0,0) += add;
        V_BE(1,1) += add;
    }

private:
    double f_;
    double xi_per_event_;
};

} // namespace nir
