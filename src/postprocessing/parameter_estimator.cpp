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

    // Модель гетеродина: y = sqrt(T)*x + noise
    // Оцениваем общий коэффициент передачи из линейной регрессии
    const double xx = x.squaredNorm();
    const double xy = x.dot(y);
    const double sqrtT_hat = xy / xx;
    const VectorR resid = y - sqrtT_hat * x;
    
    // Дисперсия остатков (шум на выходе детектора на одну квадратуру)
    const double sigma2_out = resid.squaredNorm() / static_cast<double>(n - 1);
    
    const double T_hat = sqrtT_hat * sqrtT_hat;
    
    // Корректная оценка избыточного шума xi, приведённого ко входу канала.
    // Полный шум на выходе детектора (в SNU): sigma2_out = eta * T * (1 + xi + v_vac) + v_el + v_vac_shot
    // Для гетеродина вакуумный шум вакуума = 1 (в SNU). 
    // Упрощённая модель шума на выходе: sigma2_out ≈ eta * T * (1 + xi) + v_el + 1 (shot noise of LO)
    // Однако в нашей модели QuantumChannel добавляет шум ДО детектора, а детектор добавляет свой шум.
    // Вернёмся к определению: xi — это избыточный шум канала (в SNU на входе).
    // Шум на выходе Bob: Var(n_out) = T * eta * (1 + xi) + v_el + (1-eta)*T + 1? 
    // Используем стандартную линеаризацию для калибровки:
    // sigma2_out = T * eta * (1 + xi) + v_el + 1 (если LO мощный, shot noise = 1)
    // => T * eta * xi = sigma2_out - T * eta - v_el - 1
    // => xi = (sigma2_out - 1 - v_el - T*eta) / (T*eta)
    // Примечание: В данной реализации предполагается, что sigma2_out измерен относительно нормированных единиц.
    // Если модель детектора в HeterodyneDetector уже включает shot noise (vacuum=1), то:
    // sigma2_out = T * eta * (1 + xi) + v_el + (1-T)*eta (потери) ... упростим до стандартной формы:
    // sigma2_total_input_referred = (sigma2_out - v_el - 1) / (eta * T)
    // xi_hat = sigma2_total_input_referred - 1 (вакуум) - (1-T)/T (потери квантовые)
    // Но проще через полный бюджет шума:
    // xi_hat = (sigma2_out - 1.0 - cfg_.v_el) / (cfg_.eta * std::max(T_hat, 1e-6)) - 1.0 - (1.0 - T_hat)/T_hat;
    // Стоп, учтём, что в канале шум добавляется как T*xi.
    // Правильная формула инверсии бюджета шума для гетеродина:
    // xi_hat = (sigma2_out - 1.0 - cfg_.v_el) / (cfg_.eta * std::max(T_hat, 1e-6)) - 1.0; 
    // (Здесь 1.0 — это вакуумный шум, который всегда есть на входе детектора даже при идеальном канале)
    // Дополнительно нужно вычесть шум потерь канала (1-T)/T? Нет, xi определяется как ДОПОЛНИТЕЛЬНЫЙ шум сверх потерь.
    // Стандартная формула: chi_tot = (1-T)/T + xi + (1+v_el)/(eta*T).
    // Измеренный шум на выходе (нормированный на вход): chi_meas = sigma2_out / T_hat.
    // Тогда xi_hat = chi_meas - (1-T)/T - (1+v_el)/(eta*T) - 1 (vacuum)?
    // Давайте используем прямую инверсию из модели детектора, если бы мы её знали точно.
    // Для универсальности используем оценку: xi_hat = (sigma2_out - 1.0 - cfg_.v_el) / (cfg_.eta * T_hat) - 1.0;
    // Если результат отрицательный (шум меньше вакуумного из-за статистики), обнуляем.
    
    // Уточненная формула с учетом того, что в канале шум xi добавляется к сигналу, 
    // а потери (1-T) заменяются на вакуум.
    // Полный шум на входе Боба (перед детектором): 1 (вакуум) + xi (избыточный)
    // После канала с потерями T: T*(1+xi) + (1-T)*1 = 1 + T*xi
    // После детектора с eta и v_el: eta*(1 + T*xi) + v_el + 1 (shot noise гетеродина)
    // Итого: sigma2_out = eta + eta*T*xi + v_el + 1
    // Отсюда: eta*T*xi = sigma2_out - eta - v_el - 1
    // xi = (sigma2_out - 1.0 - cfg_.v_el - cfg_.eta) / (cfg_.eta * T_hat)
    
    const double numerator = sigma2_out - 1.0 - cfg_.v_el - cfg_.eta;
    const double denominator = cfg_.eta * std::max(T_hat, 1e-9);
    const double xi_hat = std::max(0.0, numerator / denominator);

    // Доверительные интервалы (асимптотические, 95%)
    const double z = (conf_ > 0.99) ? 2.576 : 1.96;
    const double se_T = 2.0 * std::abs(sqrtT_hat) * std::sqrt(sigma2_out / xx);
    
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