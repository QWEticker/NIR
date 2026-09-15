#include "attacks/entangling_cloner_model.hpp"
#include "postprocessing/parameter_estimator.hpp"
#include "postprocessing/security_analyzer.hpp"
#include "postprocessing/statistics.hpp"
#include <Eigen/Eigenvalues>
#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

using namespace cvqkd;

namespace {
double gaussian_entropy(const MatrixR& covariance) {
    MatrixR omega = MatrixR::Zero(covariance.rows(), covariance.cols());
    for (Eigen::Index i = 0; i < omega.rows(); i += 2) {
        omega(i, i + 1) = 1.0;
        omega(i + 1, i) = -1.0;
    }
    const MatrixCd matrix = Complex(0, 1) * omega.cast<Complex>() * covariance.cast<Complex>();
    Eigen::ComplexEigenSolver<MatrixCd> eigen(matrix);
    EXPECT_EQ(eigen.info(), Eigen::Success);
    double entropy = 0.0;
    for (Eigen::Index i = 0; i < eigen.eigenvalues().size(); ++i) {
        EXPECT_NEAR(eigen.eigenvalues()(i).imag(), 0.0, 1e-9);
        const double nu = std::abs(eigen.eigenvalues()(i).real());
        EXPECT_GE(nu, 1.0 - 1e-9);
        const double n = std::max(0.0, (nu - 1.0) / 2.0);
        if (n > 0.0) entropy += (n + 1) * std::log2(n + 1) - n * std::log2(n);
    }
    return entropy / 2.0;
}
} // namespace

TEST(DistributionQuantiles, MatchStudentAndChiSquareReferenceValues) {
    EXPECT_NEAR(student_quantile(0.975, 1), 12.706204736432095, 1e-9);
    EXPECT_NEAR(student_quantile(0.975, 19), 2.093024054408263, 1e-10);
    EXPECT_NEAR(student_quantile(0.025, 19), -2.093024054408263, 1e-10);
    EXPECT_NEAR(chi_square_quantile(0.025, 19), 8.906516481987971, 1e-10);
    EXPECT_NEAR(chi_square_quantile(0.975, 19), 32.85232686172969, 1e-9);
    EXPECT_THROW(student_quantile(1.0, 19), std::invalid_argument);
    EXPECT_THROW(chi_square_quantile(0.5, 0.0), std::invalid_argument);
}

TEST(ParameterInference, RecoversTransmissionAndInputReferredNoise) {
    ProtocolConfig cfg;
    cfg.eta = 0.6;
    cfg.v_el = 0.05;
    std::mt19937_64 rng(612);
    std::normal_distribution<double> noise(0.0, std::sqrt(0.525 + 0.6 * 0.5 * 0.4 / 4));
    VectorR x(200000), y(x.size());
    for (Eigen::Index i = 0; i < x.size(); ++i) {
        x(i) = i % 2 == 0 ? 0.5 : -0.5;
        y(i) = std::sqrt(0.6 * 0.5) * x(i) + noise(rng);
    }
    const auto e = ParameterEstimator(cfg).estimate(x, y);
    EXPECT_NEAR(e.T_hat, 0.5, 0.03);
    EXPECT_NEAR(e.xi_hat, 0.4, 0.18);
    EXPECT_TRUE(e.identifiable);
    EXPECT_LT(e.T_lower, e.T_upper);
    EXPECT_LT(e.xi_lower, e.xi_upper);
    EXPECT_LT(ParameterEstimator(cfg, 0.9).estimate(x, y).T_ci_half,
              ParameterEstimator(cfg, 0.99).estimate(x, y).T_ci_half);
}

TEST(ParameterInference, PreservesNegativeNoiseAndRejectsUnidentifiableData) {
    ProtocolConfig cfg;
    cfg.eta = 1.0;
    cfg.v_el = 0.0;
    VectorR x(4), y(4);
    x << -1, -1, 1, 1;
    y = 0.5 * x;
    const auto e = ParameterEstimator(cfg).estimate(x, y);
    EXPECT_DOUBLE_EQ(e.T_hat, 0.25);
    EXPECT_DOUBLE_EQ(e.xi_hat, -8.0);
    y << -1, 1, -1, 1;
    const auto zero = ParameterEstimator(cfg).estimate(x, y);
    EXPECT_FALSE(zero.identifiable);
    EXPECT_TRUE(std::isnan(zero.xi_hat));
    EXPECT_TRUE(std::isinf(zero.xi_upper));
    EXPECT_THROW(ParameterEstimator(cfg).estimate(VectorR::Zero(4), y), std::invalid_argument);
    EXPECT_THROW(ParameterEstimator(cfg).estimate(x, VectorR::Zero(3)), std::invalid_argument);
}

TEST(GaussianSecurityValidation, MatchesFullTwoEveModePurification) {
    for (double eta : {0.6, 1.0}) {
        ProtocolConfig cfg;
        cfg.eta = eta;
        cfg.v_el = eta == 1.0 ? 0.0 : 0.05;
        for (double T : {0.01, 0.1, 0.5, 0.9, 0.99}) {
            for (double xi : {0.0, 0.01, 0.3}) {
                for (double V_A : {0.5, 4.0}) {
                    const auto cm = EntanglingClonerModel().covariance(T, V_A, xi);
                    const MatrixR eve = cm.bottomRightCorner<4, 4>();
                    const MatrixR correlations = cm.block<4, 2>(2, 0);
                    const double denominator = eta * cm(0, 0) + 2 - eta + 2 * cfg.v_el;
                    const MatrixR conditional = eve -
                        eta * correlations * correlations.transpose() / denominator;
                    const double reference = gaussian_entropy(eve) - gaussian_entropy(conditional);
                    EXPECT_NEAR(SecurityAnalyzer(cfg).holevo_bound(T, xi, V_A),
                                reference, 1e-8) << T << " " << xi << " " << V_A << " " << eta;
                }
            }
        }
    }
}

TEST(GaussianSecurityValidation, IdealAndZeroTransmissionLimits) {
    ProtocolConfig cfg;
    cfg.eta = 1.0;
    cfg.v_el = 0.0;
    const auto m = SecurityAnalyzer(cfg).compute(1.0, 0.0, 4.0);
    EXPECT_NEAR(m.I_AB, std::log2(3.0), 1e-14);
    EXPECT_NEAR(m.chi_BE, 0.0, 1e-10);
    EXPECT_NEAR(m.K_beta, cfg.beta * std::log2(3.0), 1e-10);
    EXPECT_DOUBLE_EQ(SecurityAnalyzer(cfg).compute(0.0, 0.0, 4.0).K_beta, 0.0);
    EXPECT_THROW(SecurityAnalyzer(cfg).compute(1.1, 0.0, 4.0), std::invalid_argument);
    EXPECT_THROW(SecurityAnalyzer(cfg).compute(0.5, -0.01, 4.0), std::invalid_argument);
}

TEST(GaussianSecurityValidation, KeyIdentitiesAndNoiseMonotonicity) {
    ProtocolConfig cfg;
    for (double T : {0.01, 0.2, 0.5, 0.95}) {
        double previous = std::numeric_limits<double>::infinity();
        for (double xi : {0.0, 0.01, 0.1, 1.0, 2.0}) {
            const auto m = SecurityAnalyzer(cfg).compute(T, xi, 4.0);
            EXPECT_LE(m.K_beta, previous + 1e-10);
            EXPECT_NEAR(m.K_beta, std::max(0.0, cfg.beta * m.I_AB - m.chi_BE), 1e-14);
            EXPECT_LE(m.K_beta, m.K_asymptotic + 1e-14);
            previous = m.K_beta;
        }
    }
}

TEST(QpskSecurityValidation, DistinctFromGaussianAndFullIRHasNoKey) {
    ProtocolConfig cfg;
    cfg.eta = 1.0;
    cfg.v_el = 0.0;
    for (double alpha : {0.1, 0.4, 0.8, 2.0}) {
        for (double T : {0.1, 0.6, 1.0}) {
            const double variance = 0.5 + T * 2.0 / 4.0;
            const double info = qpsk_awgn_mutual_information(alpha, std::sqrt(T), variance);
            const auto m = SecurityAnalyzer(cfg).compute_qpsk(T, 2.0, alpha, info);
            EXPECT_GE(m.chi_BE + 1e-10, m.I_AB);
            EXPECT_DOUBLE_EQ(m.K_beta, 0.0);
            EXPECT_EQ(m.model, "qpsk_denys_asymptotic_untrusted_detector");
            EXPECT_LE(info, 2.0);
        }
    }
}

TEST(QpskSecurityValidation, SourceSpectrumAndCovarianceBound) {
    for (double alpha : {0.01, 0.1, 0.4, 1.0, 5.0}) {
        const auto source = SecurityAnalyzer::qpsk_source_moments(alpha);
        double sum = 0.0;
        for (double eigenvalue : source.eigenvalues) {
            EXPECT_GT(eigenvalue, 0.0);
            sum += eigenvalue;
        }
        EXPECT_NEAR(sum, 1.0, 1e-14);
        EXPECT_GE(source.w, 0.0);
        EXPECT_LE(source.trace, alpha * std::sqrt(1 + alpha * alpha) + 1e-12);
    }
}

TEST(QpskStatistics, UniformPerfectAndErasedObservations) {
    std::vector<std::uint8_t> alice{0, 1, 2, 3};
    Measurement m{VectorR(4), VectorR(4)};
    m.X << 1, 0, -1, 0;
    m.P << 0, 1, 0, -1;
    const auto ideal = symbol_statistics(alice, m);
    EXPECT_DOUBLE_EQ(ideal.mutual_information, 2.0);
    EXPECT_DOUBLE_EQ(ideal.bit_error_rate, 0.0);
    const auto erased = symbol_statistics(alice, m, {0, 0, 0, 0});
    EXPECT_DOUBLE_EQ(erased.mutual_information, 0.0);
    EXPECT_TRUE(std::isnan(erased.bit_error_rate));
    EXPECT_EQ(erased.observed, 0u);
    EXPECT_DOUBLE_EQ(qpsk_awgn_mutual_information(0.5, 0.0, 0.5), 0.0);
    EXPECT_DOUBLE_EQ(qpsk_awgn_mutual_information(0.5, 1.0, 0.0), 2.0);
}
