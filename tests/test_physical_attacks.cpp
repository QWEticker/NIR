#include "attacks/collective_attack.hpp"
#include "attacks/collective_model.hpp"
#include "attacks/intercept_resend_attack.hpp"
#include "attacks/intercept_resend_model.hpp"
#include "protocol/heterodyne_detector.hpp"
#include "protocol/quantum_channel.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace cvqkd;

namespace {
constexpr Eigen::Index sample_count = 200000;

Measurement constant_input() {
    return {VectorR::Ones(sample_count), VectorR::Constant(sample_count, -0.25)};
}

void expect_moments(const Measurement& m, double mean_x, double mean_p, double variance) {
    const double n = static_cast<double>(m.X.size());
    const auto dx = (m.X.array() - m.X.mean()).eval();
    const auto dp = (m.P.array() - m.P.mean()).eval();
    EXPECT_NEAR(m.X.mean(), mean_x, 6 * std::sqrt(variance / n) + 1e-12);
    EXPECT_NEAR(m.P.mean(), mean_p, 6 * std::sqrt(variance / n) + 1e-12);
    EXPECT_NEAR(dx.square().sum() / (n - 1), variance,
                6 * variance * std::sqrt(2 / (n - 1)) + 1e-12);
    EXPECT_NEAR(dp.square().sum() / (n - 1), variance,
                6 * variance * std::sqrt(2 / (n - 1)) + 1e-12);
    EXPECT_NEAR((dx * dp).sum() / (n - 1), 0.0,
                6 * variance / std::sqrt(n - 1) + 1e-12);
}
} // namespace

TEST(PhysicalIR, QuantumLimitedEveRetainsHeterodyneNoise) {
    auto m = constant_input();
    InterceptResendAttack eve(1.0, 1.0, 401);
    eve.apply(m);
    expect_moments(m, 1.0, -0.25, 0.5);
    EXPECT_EQ(eve.stage(), AttackStage::Source);
    EXPECT_TRUE(m.X.isApprox(eve.eve_measurement().X, 0.0));
    const auto prediction = InterceptResendModel(1.0, 1.0).computeEffect(0.6, 0.5, 0.01);
    EXPECT_DOUBLE_EQ(prediction.xi_add, 2.0);
    EXPECT_DOUBLE_EQ(prediction.xi_total, 2.01);
    EXPECT_DOUBLE_EQ(prediction.transmission, 0.6);
}

TEST(PhysicalIR, CalibratedRealEveHasReceiverNoise) {
    auto m = constant_input();
    InterceptResendAttack eve(0.6, 0.8, 402, 0.05);
    eve.apply(m);
    expect_moments(m, 0.8, -0.2, 0.8 * 0.8 * 1.05 / 1.2);
    const auto prediction = InterceptResendModel(0.6, 0.8, 0.05).computeEffect(0.6, 0.5, 0.01);
    EXPECT_NEAR(prediction.transmission, 0.384, 1e-14);
    EXPECT_NEAR(prediction.xi_total, 0.01 / 0.64 + 3.5, 1e-14);
}

TEST(PhysicalIR, NoiselessOracleIsAnExplicitSeparateMode) {
    auto m = constant_input();
    InterceptResendAttack eve(0.6, 1.0, 403, 0.05, 1.0, 10.0, EveMode::Oracle);
    eve.apply(m);
    expect_moments(m, 1.0, -0.25, 0.0);
}

TEST(PhysicalIR, ZeroInterceptionIsIdentityAndRecordsErasures) {
    auto m = constant_input();
    const auto original = m;
    InterceptResendAttack eve(0.6, 0.8, 404, 0.05, 0.0);
    eve.apply(m);
    EXPECT_TRUE(m.X.isApprox(original.X, 0.0));
    EXPECT_TRUE(m.P.isApprox(original.P, 0.0));
    for (auto intercepted : eve.intercepted()) ASSERT_EQ(intercepted, 0);
}

TEST(PhysicalIR, PartialInterceptionAndSeedsAreReproducible) {
    auto m = constant_input(), same = m, different = m;
    InterceptResendAttack a(0.6, 1.0, 405, 0.05, 0.4);
    InterceptResendAttack b(0.6, 1.0, 405, 0.05, 0.4);
    InterceptResendAttack c(0.6, 1.0, 406, 0.05, 0.4);
    a.apply(m);
    b.apply(same);
    c.apply(different);
    EXPECT_TRUE(m.X.isApprox(same.X, 0.0));
    EXPECT_EQ(a.intercepted(), b.intercepted());
    EXPECT_FALSE(m.X.isApprox(different.X, 0.0));
    std::size_t count = 0;
    for (auto intercepted : a.intercepted()) count += intercepted;
    EXPECT_NEAR(static_cast<double>(count), 0.4 * sample_count,
                6 * std::sqrt(sample_count * 0.4 * 0.6));
}

TEST(PhysicalIR, SaturationAndZeroGainAreObservable) {
    auto m = constant_input();
    InterceptResendAttack eve(1.0, 0.0, 407, 0.0, 1.0, 0.2);
    eve.apply(m);
    EXPECT_TRUE(m.X.isZero(0.0));
    EXPECT_GT(eve.clipped_components(), 0u);
    EXPECT_LE(eve.eve_measurement().X.cwiseAbs().maxCoeff(), 0.2);
    EXPECT_TRUE(std::isnan(InterceptResendModel(1.0, 0.0).computeEffect(0.5, 0.5, 0.01).xi_total));
}

TEST(PhysicalCollective, NoiseVarianceAndAnalyticalChannelAgree) {
    auto m = constant_input();
    CollectiveAttack attack(0.3, 0.05, 408);
    attack.apply(m);
    expect_moments(m, std::sqrt(0.91), -0.25 * std::sqrt(0.91), 0.91 * 0.05 / 4);
    const auto prediction = CollectiveModel(0.3, 0.05).computeEffect(0.6, 0.5, 0.01);
    EXPECT_NEAR(prediction.transmission, 0.546, 1e-14);
    EXPECT_NEAR(prediction.xi_total, 0.01 + 0.05 / 0.6, 1e-14);
    EXPECT_EQ(attack.stage(), AttackStage::ChannelOutput);
}

TEST(PhysicalCollective, PureLossAddsNoExcessNoise) {
    auto m = constant_input();
    CollectiveAttack(0.3, 0.0, 409).apply(m);
    expect_moments(m, std::sqrt(0.91), -0.25 * std::sqrt(0.91), 0.0);
    EXPECT_DOUBLE_EQ(CollectiveModel(0.3, 0.0).computeEffect(0.6, 0.5, 0.01).xi_add, 0.0);
}

TEST(PhysicalCollective, IdentityAndVacuumOutputLimits) {
    auto m = constant_input();
    CollectiveAttack(0.0, 0.0).apply(m);
    expect_moments(m, 1.0, -0.25, 0.0);
    CollectiveAttack(1.0, 0.05).apply(m);
    EXPECT_TRUE(m.X.isZero(0.0));
    EXPECT_TRUE(m.P.isZero(0.0));
    EXPECT_TRUE(std::isnan(CollectiveModel(1.0, 0.05).computeEffect(0.6, 0.5, 0.01).xi_total));
}

TEST(PhysicalCollective, SameSeedProducesSameOpticalDisplacements) {
    auto m = constant_input(), same = m, different = m;
    CollectiveAttack(0.3, 0.05, 410).apply(m);
    CollectiveAttack(0.3, 0.05, 410).apply(same);
    CollectiveAttack(0.3, 0.05, 411).apply(different);
    EXPECT_TRUE(m.X.isApprox(same.X, 0.0));
    EXPECT_FALSE(m.X.isApprox(different.X, 0.0));
}

TEST(OpticalChain, BobNoiseIsNotAttenuatedByLossBeforeDetection) {
    ProtocolConfig cfg;
    cfg.eta = 0.6;
    cfg.v_el = 0.05;
    std::vector<Complex> vacuum(sample_count, Complex(0.0, 0.0));
    HeterodyneDetector detector(cfg);
    expect_moments(detector.measure(vacuum), 0.0, 0.0, 0.525);
    EXPECT_DOUBLE_EQ(detector.detector_noise_variance(), 0.525);
}

TEST(OpticalChain, ChannelUsesCoherentAmplitudeUnits) {
    ProtocolConfig cfg;
    cfg.T = 0.6;
    cfg.xi = 0.4;
    std::vector<Complex> input(sample_count, Complex(1.0, -0.25));
    const auto output = QuantumChannel(cfg).transmit(input);
    Measurement m = constant_input();
    for (Eigen::Index i = 0; i < sample_count; ++i) {
        m.X(i) = output[static_cast<std::size_t>(i)].real();
        m.P(i) = output[static_cast<std::size_t>(i)].imag();
    }
    expect_moments(m, std::sqrt(0.6), -0.25 * std::sqrt(0.6), 0.6 * 0.4 / 4);
}

TEST(PhysicalAttacks, RejectInvalidParametersAndMismatchedVectors) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW(CollectiveAttack(nan, 0.0), std::invalid_argument);
    EXPECT_THROW(CollectiveAttack(1.01, 0.0), std::invalid_argument);
    EXPECT_THROW(CollectiveModel(0.3, -0.1), std::invalid_argument);
    EXPECT_THROW(InterceptResendAttack(0.0), std::invalid_argument);
    EXPECT_THROW(InterceptResendAttack(1.0, 1.0, 0, 0.0, 1.1), std::invalid_argument);
    EXPECT_THROW(InterceptResendModel(1.0, -1.0), std::invalid_argument);
    Measurement m{VectorR::Ones(3), VectorR::Ones(4)};
    EXPECT_THROW(CollectiveAttack().apply(m), std::invalid_argument);
    EXPECT_THROW(InterceptResendAttack().apply(m), std::invalid_argument);
}
