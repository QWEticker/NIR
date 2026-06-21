#include "FiniteKeyAnalyzer.h"
#include <cmath>

namespace nir {

double FiniteKeyAnalyzer::finiteSizeDelta(uint64_t n, double eps_total) {
    if (n == 0) return 1.0;
    // Use approximation: delta = c * sqrt( log(1/eps_total) / n ) (in nats),
    // convert to bits by dividing by ln(2).
    const double c = 6.0; // conservative constant (tunable);
    double ln_term = std::max(1.0, std::log(1.0 / std::max(eps_total, 1e-300)));
    double delta_nats = c * std::sqrt(ln_term / static_cast<double>(n));
    double delta_bits = delta_nats / std::log(2.0);
    // Cap delta to [0,1]
    if (delta_bits < 0.0) delta_bits = 0.0;
    if (delta_bits > 1.0) delta_bits = 1.0;
    return delta_bits;
}

} // namespace nir
