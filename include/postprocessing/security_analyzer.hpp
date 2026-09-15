#pragma once
#include "core/config.hpp"
#include <array>
#include <string>

namespace cvqkd {

struct SecurityMetrics {
    double I_AB;            ///< I(A:B), бит/символ
    double chi_BE;          ///< χ(B:E), граница Холево
    double K_asymptotic;    ///< I_AB - χ_BE
    double K_beta;          ///< β·I_AB - χ_BE  (практическая)
    double xi_eff = 0.0;    ///< эффективный избыточный шум, вошедший в расчёт
    double T_used = 0.0;
    double correlation_bound = 0.0;
    double bob_photon_number = 0.0;
    bool projected_moments = false;
    bool model_supported = true;
    std::string model = "gaussian_modulation_reference";
    /// Positive asymptotic expression only; no finite key is certified.
    bool   secure() const noexcept { return model_supported && K_beta > 0.0; }
};

struct QpskSourceMoments {
    std::array<double, 4> eigenvalues{};
    double trace = 0.0;
    double w = 0.0;
};

class SecurityAnalyzer {
public:
    explicit SecurityAnalyzer(const ProtocolConfig& cfg) : cfg_(cfg) {}

    SecurityMetrics compute(double T, double xi, double V_A) const;
    SecurityMetrics compute_qpsk(double T, double xi, double alpha, double I_AB) const;
    SecurityMetrics compute_qpsk_moments(double c2, double n_b, double alpha, double I_AB) const;
    static QpskSourceMoments qpsk_source_moments(double alpha);
    double holevo_bound(double T, double xi, double V_A) const;
    static double mutual_info(double SNR);

private:
    ProtocolConfig cfg_;
};

} // namespace cvqkd