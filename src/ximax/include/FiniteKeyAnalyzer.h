#pragma once

#include <cstdint>

namespace nir {

class FiniteKeyAnalyzer {
public:
    // Finite-size correction term Delta(n, eps) in bits per symbol.
    // Uses a conservative bound derived from smooth-entropy scaling: O(sqrt(log(1/eps)/n)).
    // eps_total is the target security parameter (e.g., 1e-9).
    static double finiteSizeDelta(uint64_t n, double eps_total);
};

} // namespace nir
