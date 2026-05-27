#include "attacks/lo_manipulation_attack.hpp"
#include "attacks/saturation_attack.hpp"
#include <gtest/gtest.h>

using namespace cvqkd;

TEST(SaturationAttack, ClipsToLevel) {
    Measurement m;
    m.X.resize(3);
    m.P.resize(3);
    m.X << 100.0, -200.0, 0.5;
    m.P << -50.0, 300.0, -0.1;
    SaturationAttack a(10.0, 1.0);
    a.apply(m);
    EXPECT_DOUBLE_EQ(m.X(0), 10.0);
    EXPECT_DOUBLE_EQ(m.X(1), -10.0);
    EXPECT_DOUBLE_EQ(m.P(1), 10.0);
}

TEST(LOManipulationAttack, ScalesLinear) {
    Measurement m;
    m.X.resize(2);
    m.P.resize(2);
    m.X << 2.0, -3.0;
    m.P << 4.0, 5.0;
    LOManipulationAttack a(0.5);
    a.apply(m);
    EXPECT_DOUBLE_EQ(m.X(0), 1.0);
    EXPECT_DOUBLE_EQ(m.P(1), 2.5);
}