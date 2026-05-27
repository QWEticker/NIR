#include "postprocessing/security_analyzer.hpp"
#include <algorithm>
#include <cmath>

namespace cvqkd {

// Функция энтропии бозе-газа g(x)
static double g_entropy(double x) {
    if (x <= 1.0) return 0.0;
    const double a = (x + 1.0) / 2.0;
    const double b = (x - 1.0) / 2.0;
    return a * std::log2(a) - b * std::log2(b);
}

double SecurityAnalyzer::mutual_info(double SNR) {
    // Для QPSK + гетеродин: I(A:B) ≈ log2(1 + SNR)
    return std::max(0.0, std::log2(1.0 + SNR));
}

double SecurityAnalyzer::holevo_bound(double T, double xi, double V_A) const {
    const double V = V_A + 1.0; // Дисперсия Алисы + вакуум
    const double chi_line = (1.0 - T) / T + xi;
    const double chi_det = (1.0 + cfg_.v_el) / (T * cfg_.eta);
    const double chi_tot = chi_line + chi_det;

    // Аналитическое решение для симплектических собственных значений 4x4 матрицы
    // Для гетеродинного детектирования с ковариационной матрицей специального вида
    const double corr = std::sqrt(T) * std::sqrt(std::max(0.0, V * V - 1.0));
    const double B = T * (V + chi_tot);
    
    // След и детерминант для характеристического полинома
    const double trace_term = V * V + B * B + 2.0 * corr * corr;
    const double det_gamma = V * V * B * B - 2.0 * V * B * corr * corr + corr * corr * corr * corr;
    
    // Решаем квадратное уравнение для ν²
    const double discriminant = trace_term * trace_term - 4.0 * det_gamma;
    if (discriminant < 0.0) {
        return g_entropy(V) + g_entropy(B) - g_entropy(V - corr * corr / B);
    }
    
    const double sqrt_disc = std::sqrt(discriminant);
    const double nu1_sq = (trace_term + sqrt_disc) / 2.0;
    const double nu2_sq = (trace_term - sqrt_disc) / 2.0;
    
    const double nu1 = std::sqrt(std::max(0.0, nu1_sq));
    const double nu2 = std::sqrt(std::max(0.0, nu2_sq));
    
    double S_AB = g_entropy(nu1) + g_entropy(nu2);
    
    const double V_AgB = V - (corr * corr) / B;
    const double S_AgB = g_entropy(std::max(1.0, V_AgB));
    
    return std::max(0.0, S_AB - S_AgB);
}

SecurityMetrics SecurityAnalyzer::compute(double T, double xi, double V_A) const {
    const double chi_tot = (1.0 - T) / T + xi + (1.0 + cfg_.v_el) / (T * cfg_.eta);
    const double SNR     = V_A / chi_tot;

    SecurityMetrics m{};
    m.I_AB         = mutual_info(SNR);
    m.chi_BE       = holevo_bound(T, xi, V_A);
    m.K_asymptotic = std::max(0.0, m.I_AB - m.chi_BE);
    m.K_beta       = std::max(0.0, cfg_.beta * m.I_AB - m.chi_BE);
    return m;
}

} // namespace cvqkd
