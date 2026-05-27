#pragma once
#include "core/types.hpp"
#include <string>

namespace cvqkd {

/// Общий интерфейс для всех атак (Strategy pattern).
class IAttack {
public:
    virtual ~IAttack() = default;
    virtual void        apply(Measurement& m) const = 0;
    virtual std::string name() const = 0;
};

} // namespace cvqkd