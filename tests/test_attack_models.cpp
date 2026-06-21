#include "gtest/gtest.h"
#include "attacks/attack_model.hpp"
#include "attacks/entangling_cloner_model.hpp"
#include "attacks/intercept_resend_model.hpp"
#include "attacks/lo_manipulation_model.hpp"
#include "attacks/saturation_model.hpp"

using namespace cvqkd;

TEST(AttackModels, EntanglingClonerReturnsCM) {
    EntanglingClonerModel m(0.001);
    auto r = m.computeEffect(0.5, 0.5, 0.01);
    EXPECT_EQ(r.type, AttackResultType::Covariance);
    EXPECT_FALSE(r.modified_cm.isZero(0.0));
}

TEST(AttackModels, InterceptResendReturnsXi) {
    InterceptResendModel m(0.8, 1.0);
    auto r = m.computeEffect(0.5, 0.5, 0.01);
    EXPECT_EQ(r.type, AttackResultType::XiAdd);
    EXPECT_GE(r.xi_add, 0.0);
}

TEST(AttackModels, LOManipulationReturnsXi) {
    LOManipulationModel m(0.7);
    auto r = m.computeEffect(0.5, 0.5, 0.01);
    EXPECT_EQ(r.type, AttackResultType::XiAdd);
    EXPECT_GE(r.xi_add, 0.0);
}

TEST(AttackModels, SaturationReturnsXi) {
    SaturationModel m(0.5, 1.0);
    auto r = m.computeEffect(0.5, 0.5, 0.01);
    EXPECT_EQ(r.type, AttackResultType::XiAdd);
    EXPECT_GE(r.xi_add, 0.0);
}
