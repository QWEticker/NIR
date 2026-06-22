#include "postprocessing/parameter_estimator.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <numeric>

namespace cvqkd {

ParameterEstimator::ParameterEstimator(const ProtocolConfig& cfg, double c) 
    : cfg_(cfg), conf_(c) {}

ChannelEstimate ParameterEstimator::estimate(const VectorR& x, const VectorR& y) const {
    const Eigen::Index n = x.size();
    if (n < 2) return ChannelEstimate{};

    // Линейная модель измерения (на квадратуру): y = sqrt(eta*T)*x + n,
    // где n = sqrt(eta)*n_канал + n_детектор и
    //   Var(n) = eta*(T*xi/2) + (1 + v_el)/eta.
    // Здесь x — амплитуда Алисы (до канала), y — отсчёт гетеродина.
    const double xx = x.squaredNorm();
    const double xy = x.dot(y);
    const double slope = xy / xx;            // оценка sqrt(eta*T)
    const VectorR resid = y - slope * x;

    // Дисперсия остатков (полный шум на выходе детектора на одну квадратуру).
    const double sigma2_out = resid.squaredNorm() / static_cast<double>(n - 1);

    // Детектор откалиброван: eta и v_el известны. Восстанавливаем T канала.
    // slope^2 = eta*T  =>  T_hat = slope^2 / eta.
    const double eta = std::max(1e-12, cfg_.eta);
    const double T_hat = (slope * slope) / eta;

    // Инверсия бюджета шума для избыточного шума канала xi (полный, SNU):
    //   sigma2_out = eta*(T*xi/2) + (1 + v_el)/eta
    //   => eta*T*xi/2 = sigma2_out - (1 + v_el)/eta
    //   => xi = 2*(sigma2_out - (1 + v_el)/eta) / (eta*T) = 2*(...) / slope^2
    const double det_noise = (1.0 + cfg_.v_el) / eta;          // (1+v_el)/eta
    const double channel_noise_out = sigma2_out - det_noise;   // = eta*T*xi/2
    const double slope2 = std::max(1e-12, slope * slope);
    const double xi_hat = std::max(0.0, 2.0 * channel_noise_out / slope2);

    // Доверительные интервалы (асимптотические, 95%)
    const double z = (conf_ > 0.99) ? 2.576 : 1.96;
    // T_hat = slope^2/eta  =>  se_T = 2*|slope|*se_slope/eta, se_slope = sqrt(sigma2_out/xx).
    const double se_T = (2.0 * std::abs(slope) * std::sqrt(sigma2_out / xx)) / eta;
    
    ChannelEstimate e{};
    e.T_hat            = T_hat;
    e.T_ci_half        = se_T * z;
    e.xi_hat           = xi_hat;
    // Оценка погрешности xi через распространение ошибок (упрощенно 10% от величины или зависит от N)
    e.xi_ci_half       = xi_hat * 0.15 + (sigma2_out / std::sqrt(static_cast<double>(n))) * 0.01; 
    e.sigma2_residual  = sigma2_out;
    e.n_samples        = static_cast<std::size_t>(n);
    return e;
}

} // namespace cvqkd