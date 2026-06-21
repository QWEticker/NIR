#pragma once
#include "../include/AttackBase.h"

namespace nir {

class DetectorSaturation : public AttackBase {
public:
    // sat_level: saturation threshold in variance units (SNU)
    DetectorSaturation(double sat_level) : sat_level_(sat_level) {}

    double inducedExcessNoise() const override {
        // Heuristic mapping: lower saturation level (more likely clipping) => larger added noise.
        // We map sat_level in [0.1..100] to xi in [0.1..0.0] roughly.
        double xi = 0.1 / std::max(0.1, sat_level_);
        return std::min(0.5, xi);
    }

    void modifyCovariance(Eigen::Matrix4d &V_BE) const override {
        // apply a nonlinear clipping proxy: if Bob's variance exceeds sat_level, reduce it
        double &bxx = V_BE(0,0);
        double &bpp = V_BE(1,1);
        if (bxx > sat_level_) {
            double excess = bxx - sat_level_;
            // clipping introduces distortion; reduce diagonal and add noise term
            bxx = sat_level_ + excess * 0.5; // partially clip
            // add small additional noise to model distortion
            bxx += 0.01 * excess;
        }
        if (bpp > sat_level_) {
            double excess = bpp - sat_level_;
            bpp = sat_level_ + excess * 0.5;
            bpp += 0.01 * excess;
        }
    }

private:
    double sat_level_;
};

} // namespace nir
