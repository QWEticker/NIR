#include "HolevoCalculator.h"
#include <cmath>

namespace nir {

using Eigen::Matrix2d;
using Eigen::Matrix4d;

// Helper: small epsilon
static constexpr double EPS = 1e-12;

Matrix4d HolevoCalculator::buildEntanglingClonerCM(double T, double V_A, double xi) {
    // V is Alice mode variance: V = V_A + 1 (1 is shot-noise)
    double V = V_A + 1.0;
    // Environmental mode variance W chosen so that excess noise at Bob equals xi:
    // xi = (1 - T)/T * (W - 1)  => W = 1 + T*xi/(1-T)
    double W = 1.0;
    if (T < 1.0 - 1e-12) {
        W = 1.0 + (T * xi) / (1.0 - T);
    } else {
        // T approx 1 => channel loss negligible; set W slightly above 1 based on xi
        W = 1.0 + xi;
    }

    // Build two-mode covariance in block form
    // V_B = b * I2, V_E = e * I2, C = c * Z where Z = diag(1, -1)
    double b = T * V + (1.0 - T) * W;
    double e = (1.0 - T) * V + T * W;
    double c = std::sqrt(std::max(0.0, T * (1.0 - T))) * (V - W);

    Matrix4d Vbe;
    Vbe.setZero();
    // V_B (xB,pB)
    Vbe(0,0) = b; Vbe(1,1) = b;
    // V_E (xE,pE)
    Vbe(2,2) = e; Vbe(3,3) = e;
    // Correlations C (xB <-> xE) and (pB <-> pE) with sign flip on p
    Vbe(0,2) = c; Vbe(2,0) = c;
    Vbe(1,3) = -c; Vbe(3,1) = -c;

    return Vbe;
}

static inline double log2d(double x) { return std::log(x) / std::log(2.0); }

double HolevoCalculator::gaussianEntropy(double nu) {
    // von Neumann entropy for single-mode Gaussian with symplectic eigenvalue nu >= 1
    // S(nu) = (nu+1)/2 * log2((nu+1)/2) - (nu-1)/2 * log2((nu-1)/2)
    double ap = (nu + 1.0) * 0.5;
    double am = (nu - 1.0) * 0.5;
    // numerical guards
    if (nu < 1.0 + EPS) return 0.0;
    double s = ap * log2d(ap) - am * log2d(std::max(am, 1e-18));
    return s;
}

double HolevoCalculator::symplecticEigenvalue(const Matrix2d &V) {
    // For single mode 2x2 CM V = [[a, c],[c, b]] (symmetric), symplectic eigenvalue nu = sqrt(det(V))
    double det = V.determinant();
    if (det < 0.0) det = 0.0;
    double nu = std::sqrt(det);
    // ensure physical minimum of 1 (shot-noise units)
    if (nu < 1.0) nu = 1.0;
    return nu;
}

static Eigen::Matrix2d block(const Matrix4d &M, int r, int c) {
    Eigen::Matrix2d B;
    B(0,0) = M(r, c);     B(0,1) = M(r, c+1);
    B(1,0) = M(r+1, c);   B(1,1) = M(r+1, c+1);
    return B;
}

double HolevoCalculator::computeHolevoFromCM(const Matrix4d &V_BE) {
    // Partition V_BE as [V_B  C; C^T  V_E]
    Eigen::Matrix2d V_B = block(V_BE, 0, 0);
    Eigen::Matrix2d V_E = block(V_BE, 2, 2);
    Eigen::Matrix2d C   = block(V_BE, 0, 2);

    // S(E): symplectic eigenvalue of V_E (single mode)
    double nu_E = symplecticEigenvalue(V_E);
    double S_E = gaussianEntropy(nu_E);

    // For heterodyne measurement on B, the measurement adds 1 SNU (vacuum)
    Eigen::Matrix2d M = V_B + Eigen::Matrix2d::Identity();
    // Compute conditional covariance of E given heterodyne outcome y:
    // V_E|y = V_E - C^T * M^{-1} * C
    Eigen::Matrix2d Minv = M.inverse();
    Eigen::Matrix2d V_E_cond = V_E - C.transpose() * Minv * C;

    double nu_E_cond = symplecticEigenvalue(V_E_cond);
    double S_E_cond = gaussianEntropy(nu_E_cond);

    double chi = S_E - S_E_cond;
    if (chi < 0.0) chi = 0.0;
    return chi;
}

} // namespace nir
