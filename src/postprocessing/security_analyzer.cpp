#include "postprocessing/security_analyzer.hpp"
#include <algorithm>
#include <cmath>

namespace cvqkd {

// Энтропийная функция гауссова состояния по симплектическому собственному
// значению nu>=1:  G(x) = (x+1)log2(x+1) - x log2(x),  x = (nu-1)/2.
static double g_vn(double nu) {
    if (nu <= 1.0) return 0.0;
    const double x  = (nu - 1.0) / 2.0;
    const double xp = x + 1.0;
    return xp * std::log2(xp) - (x > 0.0 ? x * std::log2(x) : 0.0);
}

double SecurityAnalyzer::mutual_info(double SNR) {
    return std::max(0.0, std::log2(1.0 + SNR));
}

// Граница Холево χ(B:E) для когерентного CV-QKD с гетеродинным детектированием,
// обратным согласованием и доверенным детектором (Laudenbach et al. 2018;
// Fossier et al. 2009). Считается через симплектические собственные значения
// ковариационной матрицы Γ_AB и условной матрицы после измерения Боба.
double SecurityAnalyzer::holevo_bound(double T, double xi, double V_A) const {
    const double V       = V_A + 1.0;
    const double eta     = std::max(1e-12, cfg_.eta);
    const double chi_het = (2.0 - eta + 2.0 * cfg_.v_el) / eta;   // шум гетеродина
    const double chi_line = (1.0 - T) / T + xi;                  // шум канала
    const double chi_tot  = chi_line + chi_het / T;

    const double sqrtB = T * (V * chi_line + 1.0);
    const double B     = sqrtB * sqrtB;
    const double A     = V * V * (1.0 - 2.0 * T) + 2.0 * T
                       + T * T * (V + chi_line) * (V + chi_line);

    const double d1  = std::sqrt(std::max(0.0, A * A - 4.0 * B));
    const double nu1 = std::sqrt(std::max(1.0, 0.5 * (A + d1)));
    const double nu2 = std::sqrt(std::max(1.0, 0.5 * (A - d1)));

    const double denom = T * (V + chi_tot);
    const double C = (A * chi_het * chi_het + B + 1.0
                      + 2.0 * chi_het * (V * sqrtB + T * (V + chi_line))
                      + 2.0 * T * (V * V - 1.0)) / (denom * denom);
    const double Dr = (V + sqrtB * chi_het) / denom;
    const double D  = Dr * Dr;

    const double d2  = std::sqrt(std::max(0.0, C * C - 4.0 * D));
    const double nu3 = std::sqrt(std::max(1.0, 0.5 * (C + d2)));
    const double nu4 = std::sqrt(std::max(1.0, 0.5 * (C - d2)));

    const double chi = g_vn(nu1) + g_vn(nu2) - g_vn(nu3) - g_vn(nu4);
    return std::max(0.0, chi);
}

SecurityMetrics SecurityAnalyzer::compute(double T, double xi, double V_A) const {
    const double V       = V_A + 1.0;
    const double eta     = std::max(1e-12, cfg_.eta);
    const double chi_het = (2.0 - eta + 2.0 * cfg_.v_el) / eta;
    const double chi_line = (1.0 - T) / T + xi;
    const double chi_tot  = chi_line + chi_het / T;

    SecurityMetrics m{};
    m.I_AB         = std::max(0.0, std::log2((V + chi_tot) / (1.0 + chi_tot)));
    m.chi_BE       = holevo_bound(T, xi, V_A);
    m.K_asymptotic = std::max(0.0, m.I_AB - m.chi_BE);
    m.K_beta       = std::max(0.0, cfg_.beta * m.I_AB - m.chi_BE);
    return m;
}

} // namespace cvqkd
