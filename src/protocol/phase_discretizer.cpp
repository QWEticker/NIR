
#define _USE_MATH_DEFINES  // ← Обязательно ДО <cmath>
#include "protocol/phase_discretizer.hpp"
#include <cmath>

namespace cvqkd {

std::vector<std::size_t>
PhaseDiscretizer::discretize(const Measurement& m) const {
    const std::size_t N = m.size();
    std::vector<std::size_t> out(N);
    constexpr double TWO_PI = 2.0 * M_PI;

    for (std::size_t i = 0; i < N; ++i) {
        double phi = std::atan2(m.P(static_cast<Eigen::Index>(i)),
                                m.X(static_cast<Eigen::Index>(i)));
        if (phi < 0.0) phi += TWO_PI;
        out[i] = static_cast<std::size_t>(phi / TWO_PI * bins_) % bins_;
    }
    return out;
}

} // namespace cvqkd