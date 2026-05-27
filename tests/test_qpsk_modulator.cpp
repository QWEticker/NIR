#include "protocol/qpsk_modulator.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace cvqkd;

TEST(QPSKModulator, ProducesFourStatesOnly) {
    QPSKModulator m(0.5, 123);
    auto s = m.generate(10'000);
    EXPECT_EQ(s.keys.size(), 10'000u);
    EXPECT_EQ(s.amplitudes.size(), 10'000u);
    for (auto a : s.amplitudes) {
        double r = std::abs(a);
        EXPECT_NEAR(r, 0.5, 1e-12);
    }
}

TEST(QPSKModulator, ModulationVariance) {
    QPSKModulator m(0.7, 1);
    EXPECT_DOUBLE_EQ(m.modulation_variance(), 0.49);
}