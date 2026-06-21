#include "ximax/include/ChannelUtils.h"
#include <gtest/gtest.h>

using namespace nir;

TEST(ChannelUtils, TransmissivityValues) {
    double T = transmissivityFromDistance(5.0, 0.5);
    EXPECT_NEAR(T, 0.562341325, 1e-6);
    double T2 = transmissivityFromDistance(50.0, 0.2);
    EXPECT_NEAR(T2, 0.1, 1e-9);
}
