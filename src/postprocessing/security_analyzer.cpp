#include "postprocessing/security_analyzer.hpp"
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <algorithm>
#include <cmath>
#include <vector>

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

    // Ковариационная матрица γ_AB для гетеродина (4x4)
    Eigen::Matrix4d g = Eigen::Matrix4d::Identity() * V;
    const double corr = std::sqrt(T) * std::sqrt(V * V - 1.0);
    g(0, 2) = g(2, 0) =  corr;
    g(1, 3) = g(3, 1) = -corr;
    g(2, 2) = g(3, 3) = T * (V + chi_tot);

    // Симплектическая форма Ω
    Eigen::Matrix4d Om = Eigen::Matrix4d::Zero();
    Om(0, 1) =  1.0; Om(1, 0) = -1.0;
    Om(2, 3) =  1.0; Om(3, 2) = -1.0;

    // Собственные значения iΩγ
    Eigen::EigenSolver<Eigen::Matrix4d> es(Om * g, false);
    std::vector<double> nu;
    for (const auto& ev : es.eigenvalues()) {
        const double a = std::abs(ev.imag());
        if (a > 1e-9) nu.push_back(a);
    }
    std::sort(nu.begin(), nu.end());
    // Оставляем уникальные с точностью
    nu.erase(std::unique(nu.begin(), nu.end(),
        [](double a, double b){ return std::abs(a-b) < 1e-6; }), nu.end());

    // Для гетеродина обычно два ненулевых симплектических корня
    double S_AB = 0.0;
    for (double n : nu) S_AB += g_entropy(n);

    // Условная энтропия S(A|B)
    const double V_AgB = V - (corr * corr) / (T * (V + chi_tot));
    const double S_AgB = g_entropy(V_AgB);
    
    // χ(B:E) = S_AB - S_AgB
    return std::max(0.0, S_AB - S_AgB);
}

SecurityMetrics SecurityAnalyzer::compute(double T, double xi, double V_A) const {
    // Общий шум, приведённый ко входу
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