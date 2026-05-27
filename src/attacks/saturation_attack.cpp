#include "attacks/saturation_attack.hpp"
#include <algorithm>

namespace cvqkd {

void SaturationAttack::apply(Measurement &m) const {
    auto clip = [this](double x) {
        x *= gain_;
        return std::clamp(x, -sat_, sat_);
    };
    for (Eigen::Index i = 0; i < m.X.size(); ++i) {
        m.X(i) = clip(m.X(i));
        m.P(i) = clip(m.P(i));
    }
}

} // namespace cvqkd