#pragma once

#include <Eigen/Dense>
#include <vector>

namespace nir {

class HolevoCalculator {
public:
    // Build the two-mode covariance matrix V_BE (4x4) for an entangling-cloner
    // model given channel transmissivity T, modulation variance V_A (SNU),
    // and excess noise xi (SNU). Returns V_BE as 4x4 Eigen::Matrix4d in the
    // ordering (x_B, p_B, x_E, p_E).
    static Eigen::Matrix4d buildEntanglingClonerCM(double T, double V_A, double xi);

    // Compute Holevo quantity chi(B;E) for heterodyne measurement at Bob using
    // the provided covariance matrix V_BE (4x4). Implements chi = S(E) - S(E|B),
    // where S(...) are von Neumann entropies of Gaussian states computed from
    // symplectic eigenvalues.
    static double computeHolevoFromCM(const Eigen::Matrix4d &V_BE);

private:
    // von Neumann entropy function for a single-mode with symplectic eigenvalue nu
    static double gaussianEntropy(double nu);

    // Compute symplectic eigenvalue for a 2x2 covariance matrix (single mode)
    static double symplecticEigenvalue(const Eigen::Matrix2d &V);
};

} // namespace nir
