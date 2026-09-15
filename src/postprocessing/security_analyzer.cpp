#include "postprocessing/security_analyzer.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cvqkd {

// Энтропийная функция гауссова состояния по симплектическому собственному
// значению nu>=1:  G(x) = (x+1)log2(x+1) - x log2(x),  x = (nu-1)/2.
static double g_vn(double nu) {
    if (nu <= 1.0) return 0.0;
    const double x  = (nu - 1.0) / 2.0;
    return std::log1p(x) / std::log(2.0) + x * std::log1p(1.0 / x) / std::log(2.0);
}

double SecurityAnalyzer::mutual_info(double SNR) {
    if (!std::isfinite(SNR) || SNR < 0.0)
        throw std::invalid_argument("SNR must be finite and nonnegative");
    return std::log1p(SNR) / std::log(2.0);
}

static void validate_security(const ProtocolConfig& cfg, double T, double xi, double V_A) {
    if (!std::isfinite(T) || T < 0.0 || T > 1.0 ||
        !std::isfinite(xi) || xi < 0.0 || !std::isfinite(V_A) || V_A < 0.0 ||
        !std::isfinite(cfg.eta) || cfg.eta <= 0.0 || cfg.eta > 1.0 ||
        !std::isfinite(cfg.v_el) || cfg.v_el < 0.0 ||
        !std::isfinite(cfg.beta) || cfg.beta < 0.0 || cfg.beta > 1.0)
        throw std::invalid_argument("invalid security model parameters");
}

// Граница Холево χ(B:E) для когерентного CV-QKD с гетеродинным детектированием,
// обратным согласованием и доверенным детектором (Laudenbach et al. 2018;
// Fossier et al. 2009). Считается через симплектические собственные значения
// ковариационной матрицы Γ_AB и условной матрицы после измерения Боба.
double SecurityAnalyzer::holevo_bound(double T, double xi, double V_A) const {
    validate_security(cfg_, T, xi, V_A);
    if (T == 0.0) return 0.0;
    const double V       = V_A + 1.0;
    const double eta     = cfg_.eta;
    const double chi_het = (2.0 - eta + 2.0 * cfg_.v_el) / eta;   // шум гетеродина
    const double b = 1.0 + T * (V_A + xi);
    const double c2 = T * V_A * (V_A + 2.0);
    const double sqrtB = V * b - c2;
    const double B     = sqrtB * sqrtB;
    const double A     = V * V + b * b - 2.0 * c2;

    const double d1  = std::sqrt(std::max(0.0, A * A - 4.0 * B));
    const double nu1 = std::sqrt(std::max(1.0, 0.5 * (A + d1)));
    const double nu2 = std::sqrt(std::max(1.0, B / (nu1 * nu1)));

    const double denom = b + chi_het;
    const double C = (A * chi_het * chi_het + B + 1.0
                      + 2.0 * chi_het * (V * sqrtB + b)
                      + 2.0 * c2) / (denom * denom);
    const double Dr = (V + sqrtB * chi_het) / denom;
    const double D  = Dr * Dr;

    const double d2  = std::sqrt(std::max(0.0, C * C - 4.0 * D));
    const double nu3 = std::sqrt(std::max(1.0, 0.5 * (C + d2)));
    const double nu4 = std::sqrt(std::max(1.0, D / (nu3 * nu3)));

    const double chi = g_vn(nu1) + g_vn(nu2) - g_vn(nu3) - g_vn(nu4);
    return std::max(0.0, chi);
}

SecurityMetrics SecurityAnalyzer::compute(double T, double xi, double V_A) const {
    validate_security(cfg_, T, xi, V_A);
    SecurityMetrics m{};
    m.T_used       = T;
    m.xi_eff       = xi;
    m.I_AB         = mutual_info(cfg_.eta * T * V_A /
                                 (2.0 + 2.0 * cfg_.v_el + cfg_.eta * T * xi));
    m.chi_BE       = holevo_bound(T, xi, V_A);
    m.K_asymptotic = std::max(0.0, m.I_AB - m.chi_BE);
    m.K_beta       = std::max(0.0, cfg_.beta * m.I_AB - m.chi_BE);
    return m;
}

QpskSourceMoments SecurityAnalyzer::qpsk_source_moments(double alpha) {
    if (!std::isfinite(alpha) || alpha <= 0.0 || !std::isfinite(2 * alpha * alpha))
        throw std::invalid_argument("QPSK requires finite positive amplitude and energy");
    const long double energy = static_cast<long double>(alpha) * alpha;
    std::array<long double, 4> p{};
    if (energy < 1.0L) {
        long double term = std::exp(-energy);
        for (std::size_t n = 0; n < 80; ++n) {
            p[n % 4] += term;
            term *= energy / (n + 1);
        }
    } else {
        const long double e = std::exp(-energy), e2 = e * e;
        p = {(1 + e2 + 2 * e * std::cos(energy)) / 4,
             (1 - e2 + 2 * e * std::sin(energy)) / 4,
             (1 + e2 - 2 * e * std::cos(energy)) / 4,
             (1 - e2 - 2 * e * std::sin(energy)) / 4};
    }
    std::array<long double, 4> ratio{};
    long double mean = 0.0L, variance = 0.0L;
    QpskSourceMoments result;
    for (std::size_t k = 0; k < 4; ++k) {
        if (p[k] <= 0 || p[(k + 1) % 4] <= 0)
            throw std::underflow_error("QPSK source below numerical precision");
        ratio[k] = std::sqrt(p[k] / p[(k + 1) % 4]);
        mean += p[k] * ratio[k];
        result.eigenvalues[k] = static_cast<double>(p[k]);
    }
    for (std::size_t k = 0; k < 4; ++k)
        variance += p[k] * (ratio[k] - mean) * (ratio[k] - mean);
    result.trace = static_cast<double>(energy * mean);
    result.w = static_cast<double>(energy * variance);
    return result;
}

SecurityMetrics SecurityAnalyzer::compute_qpsk(double T, double xi, double alpha, double I_AB) const {
    validate_security(cfg_, T, xi, 2 * alpha * alpha);
    const double n = alpha * alpha;
    const double t = cfg_.eta * T;
    return compute_qpsk_moments(std::sqrt(t) * n, t * (n + xi / 2.0) + cfg_.v_el,
                                alpha, I_AB);
}

SecurityMetrics SecurityAnalyzer::compute_qpsk_moments(double c2, double n_b, double alpha,
                                                       double I_AB) const {
    validate_security(cfg_, 0.0, 0.0, 2 * alpha * alpha);
    if (!std::isfinite(c2) || !std::isfinite(n_b) ||
        !std::isfinite(I_AB) || I_AB < 0.0 || I_AB > 2.0 + 1e-12)
        throw std::invalid_argument("invalid observed QPSK moments or mutual information");
    const auto source = qpsk_source_moments(alpha);
    const double energy = alpha * alpha;
    if (energy == 0.0) throw std::underflow_error("QPSK energy below numerical precision");
    const double explained_energy = c2 * c2 / energy;
    const double physical_n = std::max(n_b, explained_energy);
    const double z = std::max(0.0, 2.0 * source.trace * c2 / energy -
                                   2.0 * std::sqrt(source.w * (physical_n - explained_energy)));
    const double a = 1.0 + 2.0 * energy, b = 1.0 + 2.0 * physical_n;
    const double discriminant = (a + b) * (a + b) - 4.0 * z * z;
    if (!std::isfinite(discriminant) || discriminant < 0.0)
        throw std::domain_error("QPSK covariance is not physical");
    const double root = std::sqrt(discriminant);
    const double nu1 = (root + a - b) / 2.0;
    const double nu2 = (root - a + b) / 2.0;
    const double conditional = a - z * z / (b + 1.0);
    if (std::min({nu1, nu2, conditional}) < 1.0 - 1e-9)
        throw std::domain_error("QPSK covariance violates uncertainty relation");
    SecurityMetrics m{};
    m.model = "qpsk_denys_asymptotic_untrusted_detector";
    m.projected_moments = n_b < explained_energy;
    m.T_used = explained_energy / (energy * cfg_.eta);
    m.xi_eff = m.T_used > 0.0 ?
        2 * (physical_n - explained_energy - cfg_.v_el) / (cfg_.eta * m.T_used) : 0.0;
    m.correlation_bound = z;
    m.bob_photon_number = physical_n;
    m.I_AB = I_AB;
    m.chi_BE = std::max(0.0, g_vn(nu1) + g_vn(nu2) - g_vn(conditional));
    m.K_asymptotic = std::max(0.0, I_AB - m.chi_BE);
    m.K_beta = std::max(0.0, cfg_.beta * I_AB - m.chi_BE);
    return m;
}

} // namespace cvqkd
