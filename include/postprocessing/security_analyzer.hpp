#pragma once
#include "core/config.hpp"

namespace cvqkd {

struct SecurityMetrics {
    double I_AB;            ///< I(A:B), бит/символ
    double chi_BE;          ///< χ(B:E), граница Холево
    double K_asymptotic;    ///< I_AB - χ_BE
    double K_beta;          ///< β·I_AB - χ_BE  (практическая)
    double xi_eff = 0.0;    ///< эффективный избыточный шум, вошедший в расчёт
    bool   secure() const noexcept { return K_beta > 0.0; }
};

class SecurityAnalyzer {
public:
    explicit SecurityAnalyzer(const ProtocolConfig& cfg) : cfg_(cfg) {}

    SecurityMetrics compute(double T, double xi, double V_A) const;
    double holevo_bound(double T, double xi, double V_A) const;
    static double mutual_info(double SNR);

private:
    ProtocolConfig cfg_;
};

} // namespace cvqkd