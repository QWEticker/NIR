#include <gtest/gtest.h>
#include "postprocessing/security_analyzer.hpp"

using namespace cvqkd;

TEST(SecurityAnalyzer, PositiveKeyAtShortDistance) {
    ProtocolConfig c;
    c.T = 0.5; c.xi = 0.01; c.eta = 0.6; c.v_el = 0.05; c.beta = 0.95;
    SecurityAnalyzer s(c);
    auto m = s.compute(c.T, c.xi, 0.25);
    EXPECT_GT(m.K_beta, 0.0);
}

TEST(SecurityAnalyzer, ZeroKeyAtHighLoss) {
    ProtocolConfig c;
    c.T = 1e-4; c.xi = 0.1; c.eta = 0.6; c.v_el = 0.05; c.beta = 0.95;
    SecurityAnalyzer s(c);
    auto m = s.compute(c.T, c.xi, 0.25);
    EXPECT_DOUBLE_EQ(m.K_beta, 0.0);
}