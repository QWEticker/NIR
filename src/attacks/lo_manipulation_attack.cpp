#include "attacks/lo_manipulation_attack.hpp"

namespace cvqkd {

void LOManipulationAttack::apply(Measurement &m) const {
    m.X *= scale_;
    m.P *= scale_;
}

} // namespace cvqkd