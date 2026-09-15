#include "postprocessing/statistics.hpp"
#include <Eigen/Eigenvalues>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cvqkd {
namespace {
constexpr double tiny = 1e-300;
constexpr double epsilon = 2e-14;

double nonzero(double x) {
    return std::abs(x) < tiny ? std::copysign(tiny, x) : x;
}

double beta_fraction(double a, double b, double x) {
    double c = 1.0;
    double d = 1.0 / nonzero(1.0 - (a + b) * x / (a + 1.0));
    double h = d;
    for (int m = 1; m <= 10000; ++m) {
        const double k = static_cast<double>(m);
        double aa = k * (b - k) * x / ((a + 2 * k - 1) * (a + 2 * k));
        d = 1.0 / nonzero(1.0 + aa * d);
        c = nonzero(1.0 + aa / c);
        h *= d * c;
        aa = -(a + k) * (a + b + k) * x / ((a + 2 * k) * (a + 2 * k + 1));
        d = 1.0 / nonzero(1.0 + aa * d);
        c = nonzero(1.0 + aa / c);
        const double delta = d * c;
        h *= delta;
        if (std::abs(delta - 1.0) < epsilon) return h;
    }
    throw std::runtime_error("incomplete beta failed to converge");
}

double beta_cdf(double x, double a, double b) {
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;
    const double factor = std::exp(std::lgamma(a + b) - std::lgamma(a) -
                                   std::lgamma(b) + a * std::log(x) + b * std::log1p(-x));
    if (x < (a + 1.0) / (a + b + 2.0)) return factor * beta_fraction(a, b, x) / a;
    return 1.0 - factor * beta_fraction(b, a, 1.0 - x) / b;
}

double gamma_cdf(double x, double a) {
    if (x <= 0.0) return 0.0;
    const double factor = std::exp(-x + a * std::log(x) - std::lgamma(a));
    if (x < a + 1.0) {
        double term = 1.0 / a, sum = term;
        for (int n = 1; n <= 100000; ++n) {
            term *= x / (a + n);
            sum += term;
            if (std::abs(term) < epsilon * std::abs(sum)) return sum * factor;
        }
    } else {
        double b = x + 1.0 - a, c = 1.0 / tiny;
        double d = 1.0 / nonzero(b), h = d;
        for (int n = 1; n <= 100000; ++n) {
            const double an = -static_cast<double>(n) * (n - a);
            b += 2.0;
            d = 1.0 / nonzero(an * d + b);
            c = nonzero(b + an / c);
            const double delta = d * c;
            h *= delta;
            if (std::abs(delta - 1.0) < epsilon) return 1.0 - factor * h;
        }
    }
    throw std::runtime_error("incomplete gamma failed to converge");
}

void validate_quantile(double p, double df) {
    if (!std::isfinite(p) || p <= 0.0 || p >= 1.0 ||
        !std::isfinite(df) || df <= 0.0)
        throw std::invalid_argument("quantile requires 0 < probability < 1 and df > 0");
}
} // namespace

double student_quantile(double p, double df) {
    validate_quantile(p, df);
    if (p == 0.5) return 0.0;
    if (p < 0.5) return -student_quantile(1.0 - p, df);
    const auto cdf = [df](double x) {
        return 1.0 - 0.5 * beta_cdf(df / (df + x * x), df / 2.0, 0.5);
    };
    double low = 0.0, high = 1.0;
    while (cdf(high) < p) {
        high *= 2;
        if (!std::isfinite(high)) throw std::overflow_error("Student quantile overflow");
    }
    for (int i = 0; i < 80; ++i) {
        const double mid = (low + high) / 2;
        if (cdf(mid) < p) low = mid; else high = mid;
    }
    return (low + high) / 2;
}

double chi_square_quantile(double p, double df) {
    validate_quantile(p, df);
    double low = 0.0, high = std::max(1.0, df);
    while (gamma_cdf(high / 2.0, df / 2.0) < p) {
        high *= 2;
        if (!std::isfinite(high)) throw std::overflow_error("chi-square quantile overflow");
    }
    for (int i = 0; i < 80; ++i) {
        const double mid = (low + high) / 2;
        if (gamma_cdf(mid / 2.0, df / 2.0) < p) low = mid; else high = mid;
    }
    return (low + high) / 2;
}

double qpsk_awgn_mutual_information(double alpha, double gain, double variance) {
    if (!std::isfinite(alpha) || alpha < 0.0 || !std::isfinite(gain) || gain < 0.0 ||
        !std::isfinite(variance) || variance < 0.0 || !std::isfinite(alpha * gain))
        throw std::invalid_argument("invalid QPSK AWGN model");
    if (gain == 0.0 || alpha == 0.0) return 0.0;
    if (variance == 0.0) return 2.0;
    constexpr Eigen::Index order = 48;
    static const Eigen::SelfAdjointEigenSolver<MatrixR> quadrature = [] {
        MatrixR jacobi = MatrixR::Zero(order, order);
        for (Eigen::Index i = 1; i < order; ++i)
            jacobi(i - 1, i) = jacobi(i, i - 1) = std::sqrt(static_cast<double>(i));
        return Eigen::SelfAdjointEigenSolver<MatrixR>(jacobi);
    }();
    if (quadrature.info() != Eigen::Success)
        throw std::runtime_error("normal quadrature eigensolver failed");
    const double amplitude = alpha * gain;
    const std::array<Complex, 4> means{
        Complex(amplitude, 0), Complex(0, amplitude),
        Complex(-amplitude, 0), Complex(0, -amplitude)};
    double conditional_entropy = 0.0;
    for (Eigen::Index i = 0; i < order; ++i) {
        for (Eigen::Index j = 0; j < order; ++j) {
            const Complex noise = std::sqrt(variance) *
                Complex(quadrature.eigenvalues()(i), quadrature.eigenvalues()(j));
            const Complex observation = means[0] + noise;
            std::array<double, 4> log_ratios{};
            for (std::size_t k = 0; k < 4; ++k)
                log_ratios[k] = (std::norm(noise) - std::norm(observation - means[k])) /
                                (2.0 * variance);
            const double largest = *std::max_element(log_ratios.begin(), log_ratios.end());
            double sum = 0.0;
            for (double value : log_ratios) sum += std::exp(value - largest);
            const double weight = std::pow(quadrature.eigenvectors()(0, i), 2) *
                                   std::pow(quadrature.eigenvectors()(0, j), 2);
            conditional_entropy += weight * (largest + std::log(sum)) / std::log(2.0);
        }
    }
    return std::clamp(2.0 - conditional_entropy, 0.0, 2.0);
}

std::size_t qpsk_decision(double x, double p) {
    if (!std::isfinite(x) || !std::isfinite(p))
        throw std::invalid_argument("QPSK decision requires finite components");
    if (std::abs(x) >= std::abs(p)) return x >= 0.0 ? 0 : 2;
    return p >= 0.0 ? 1 : 3;
}

SymbolStatistics symbol_statistics(const std::vector<std::uint8_t>& alice,
                                   const Measurement& receiver,
                                   const std::vector<std::uint8_t>& observed) {
    if (receiver.X.size() != receiver.P.size() || receiver.size() != alice.size() ||
        (!observed.empty() && observed.size() != alice.size()) || alice.empty())
        throw std::invalid_argument("symbol statistics require matching nonempty vectors");
    SymbolStatistics result;
    std::array<std::uint64_t, 4> rows{};
    std::array<std::uint64_t, 5> columns{};
    std::uint64_t symbol_errors = 0, bit_errors = 0;
    for (std::size_t i = 0; i < alice.size(); ++i) {
        if (alice[i] >= 4 || (!observed.empty() && observed[i] > 1))
            throw std::invalid_argument("invalid QPSK label or observation mask");
        const bool present = observed.empty() || observed[i] != 0;
        const auto k = present ? qpsk_decision(receiver.X(static_cast<Eigen::Index>(i)),
                                               receiver.P(static_cast<Eigen::Index>(i))) : 4;
        ++result.counts[alice[i]][k];
        ++rows[alice[i]];
        ++columns[k];
        ++result.total;
        if (present) {
            ++result.observed;
            symbol_errors += static_cast<std::uint64_t>(alice[i] != k);
            const auto gray_xor = (alice[i] ^ (alice[i] >> 1)) ^ (k ^ (k >> 1));
            bit_errors += (gray_xor & 1) + ((gray_xor >> 1) & 1);
        }
    }
    const double total = static_cast<double>(result.total);
    for (std::size_t a = 0; a < 4; ++a) {
        for (std::size_t b = 0; b < 5; ++b) {
            const double count = static_cast<double>(result.counts[a][b]);
            if (count > 0.0)
                result.mutual_information += count / total *
                    std::log2(count * total / (static_cast<double>(rows[a]) * columns[b]));
        }
    }
    result.mutual_information = std::max(0.0, result.mutual_information);
    if (result.observed != 0) {
        result.symbol_error_rate = static_cast<double>(symbol_errors) / result.observed;
        result.bit_error_rate = static_cast<double>(bit_errors) / (2.0 * result.observed);
    }
    return result;
}

} // namespace cvqkd
