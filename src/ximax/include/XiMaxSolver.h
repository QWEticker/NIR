#pragma once

#include <cstdint>
#include <Eigen/Dense>

namespace nir {

struct XiMaxResult {
    double xi_max; // maximum tolerable excess noise (SNU)
    bool success;
};

class XiMaxSolver {
public:
    static XiMaxResult solve(double L,
                             double alpha_db_per_km,
                             double V_A,
                             double eta,
                             double v_el,
                             double beta,
                             uint64_t n,
                             double eps_total,
                             double xi_tol = 1e-6,
                             int max_iter = 80);
};

} // namespace nir
