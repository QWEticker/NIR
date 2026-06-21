#pragma once

#include "../include/AttackBase.h"
#include <memory>
#include <string>

namespace nir {

class AttackRegistry {
public:
    static std::shared_ptr<AttackBase> createByName(const std::string &name, double param) {
        if (name == "entangling_cloner") return std::make_shared<EntanglingCloner>(param);
        if (name == "lo_manipulation") return std::make_shared<LOManipulation>(param, 0.0);
        if (name == "detector_saturation") return std::make_shared<DetectorSaturation>(param);
        if (name == "intercept_resend") return std::make_shared<InterceptResend>(param, 1.0);
        return nullptr;
    }
};

} // namespace nir
