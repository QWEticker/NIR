#pragma once
#include "../include/AttackBase.h"

namespace nir {

class LOManipulation : public AttackBase {
public:
    // delta_amp: fractional change in LO amplitude (e.g., 0.1 -> +10%)
    // delta_phase: additive phase offset in radians
    LOManipulation(double delta_amp=0.0, double delta_phase=0.0) : da_(delta_amp), dp_(delta_phase) {}

    double inducedExcessNoise() const override {
        // LO amplitude scaling changes shot-noise normalization by factor (1+da_)^2.
        // Miscalibration leads to apparent excess noise approximately proportional
        // to relative change squared; phase offsets also contribute.
        double amp_term = da_ * da_;
        double phase_term = dp_ * dp_;
        // scale factor chosen conservatively; can be tuned with calibration modeling
        double xi = 0.5 * (amp_term + phase_term);
        return xi;
    }

    void modifyCovariance(Eigen::Matrix4d &V_BE) const override {
        // LO amplitude change effectively rescales Bob's measured quadratures.
        double scale = 1.0 + da_;
        V_BE(0,0) *= (scale * scale);
        V_BE(1,1) *= (scale * scale);
        // small phase-induced mixing: add a tiny cross-term proportional to phase
        double mix = dp_ * 0.01;
        V_BE(0,1) += mix;
        V_BE(1,0) += mix;
    }

private:
    double da_;
    double dp_;
};

} // namespace nir
