#include "ChannelUtils.h"
#include <gtest/gtest.h>

using namespace nir;

TEST(ChannelUtils, TransmissivityValues) {
    // alpha = 0.5 dB/km, L = 5 km -> T ~ 10^{-0.5*5/10} = 10^{-0.25}
    double T = transmissivityFromDistance(5.0, 0.5);
    // expected approx 0.562341
    EXPECT_NEAR(T, 0.562341325, 1e-6);

    // alpha = 0.2 dB/km, L = 50 km -> T ~ 10^{-0.2*50/10} = 10^{-1} = 0.1
    double T2 = transmissivityFromDistance(50.0, 0.2);
    EXPECT_NEAR(T2, 0.1, 1e-9);
}
