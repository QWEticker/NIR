#include <gtest/gtest.h>
#include "ximax/include/HolevoCalculator.h"
#include "ximax/include/FiniteKeyAnalyzer.h"
#include "ximax/include/XiMaxSolver.h"

using namespace nir;

TEST(HolevoCompute, BasicPhysicalValues) {
    double T = 0.5;
    double V_A = 1.0;
    double xi = 0.01;
    Eigen::Matrix4d Vbe = HolevoCalculator::buildEntanglingClonerCM(T, V_A, xi);
    double chi = HolevoCalculator::computeHolevoFromCM(Vbe);
    EXPECT_GE(chi, 0.0);
}

TEST(XiMaxSolver, BasicScenario) {
    double L = 10.0;
    double alpha = 0.2;
    double V_A = 0.5;
    double eta = 0.65;
    double v_el = 0.01;
    double beta = 0.94;
    uint64_t n = 1000000;
    double eps = 1e-9;

    auto res = XiMaxSolver::solve(L, alpha, V_A, eta, v_el, beta, n, eps);
    EXPECT_TRUE(res.success);
    // xi_max may be small but should be non-negative
    EXPECT_GE(res.xi_max, 0.0);
}
