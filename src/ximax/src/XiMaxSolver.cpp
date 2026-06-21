#include "XiMaxSolver.h"
#include "HolevoCalculator.h"
#include "FiniteKeyAnalyzer.h"
#include "HeterodyneReceiverDetailed.h"
#include <cmath>

namespace nir {

static inline double transmissivityFromDistance(double L, double alpha_db_per_km) {
    return std::pow(10.0, -alpha_db_per_km * L / 10.0);
}

XiMaxResult XiMaxSolver::solve(double L, double alpha_db_per_km, double V_A, double eta, double v_el, double beta, uint64_t n, double eps_total, double xi_tol, int max_iter) {
    XiMaxResult res;
    res.success = false;
    double T = transmissivityFromDistance(L, alpha_db_per_km);

    // Compute detector referred noise v_det using HeterodyneReceiverDetailed helper
    HeterodyneReceiverDetailed recv(eta, v_el);
    double v_det = recv.computeVdet();

    // Binary search for xi_max
    double xi_low = 0.0;
    double xi_high = 1.0; // SNU

    auto keyrate_for_xi = [&](double xi)->double {
        // Build covariance matrix for entangling cloner
        Eigen::Matrix4d Vbe = HolevoCalculator::buildEntanglingClonerCM(T, V_A, xi + 0.0);
        // compute Holevo chi(B;E) for heterodyne at Bob
        double chi = HolevoCalculator::computeHolevoFromCM(Vbe);

        // Compute Bob's SNR and mutual information (Gaussian approx)
        double signal = T * V_A;
        double noise = 1.0 + T * xi + v_det;
        double snr = (noise > 0.0) ? signal / noise : 0.0;
        double Iab = 0.5 * std::log2(1.0 + snr);

        double delta = FiniteKeyAnalyzer::finiteSizeDelta(n, eps_total);
        double K = beta * Iab - chi - delta;
        return K;
    };

    // Quick feasibility check
    if (keyrate_for_xi(0.0) <= 0.0) {
        res.xi_max = 0.0;
        res.success = false;
        return res;
    }

    // expand high until keyrate <= 0
    for (int i = 0; i < 30; ++i) {
        if (keyrate_for_xi(xi_high) <= 0.0) break;
        xi_high *= 2.0;
        if (xi_high > 100.0) break;
    }

    for (int iter = 0; iter < max_iter; ++iter) {
        double xi_mid = 0.5 * (xi_low + xi_high);
        double k = keyrate_for_xi(xi_mid);
        if (k > 0.0) xi_low = xi_mid;
        else xi_high = xi_mid;
        if (std::abs(xi_high - xi_low) < xi_tol) break;
    }

    res.xi_max = xi_low;
    res.success = true;
    return res;
}

} // namespace nir
