#include "postprocessing/parameter_estimator.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <numeric>

namespace cvqkd {

ParameterEstimator::ParameterEstimator(double c) : conf_(c) {}

ChannelEstimate ParameterEstimator::estimate(const VectorR& x, const VectorR& y) const {
    const Eigen::Index n = x.size();
    if (n < 2) return ChannelEstimate{};

    // Модель гетеродина: y = sqrt(T)*x + noise
    const double xx = x.squaredNorm();
    const double xy = x.dot(y);
    const double sqrtT_hat = xy / xx;
    const VectorR resid = y - sqrtT_hat * x;
    
    // Дисперсия шума на выходе детектора (на одну квадратуру)
    const double sigma2_out = resid.squaredNorm() / static_cast<double>(n - 1);
    
    // Приводим шум ко входу канала: chi_tot = (sigma2_out / eta) - 1
    // Но eta уже учтён в детекторе, поэтому используем упрощённую оценку:
    // xi_hat = 2 * (sigma2_out / T_hat - 1)  (для гетеродина с учётом +1 SNU вакуума)
    const double T_hat = sqrtT_hat * sqrtT_hat;
    const double xi_hat = std::max(0.0, 2.0 * (sigma2_out / std::max(T_hat, 1e-6) - 1.0));

    // Доверительные интервалы (асимптотические, 95%)
    const double z = (conf_ > 0.99) ? 2.576 : 1.96;
    const double se_T = 2.0 * std::abs(sqrtT_hat) * std::sqrt(sigma2_out / xx);
    
    ChannelEstimate e{};
    e.T_hat            = T_hat;
    e.T_ci_half        = se_T * z;
    e.xi_hat           = xi_hat;
    e.xi_ci_half       = xi_hat * 0.1; // Упрощённая оценка для отчёта
    e.sigma2_residual  = sigma2_out;
    e.n_samples        = static_cast<std::size_t>(n);
    return e;
}

} // namespace cvqkd