#pragma once
#include "core/config.hpp"
#include "core/types.hpp"
#include <utility>

namespace cvqkd 
{

struct ChannelEstimate {
    double T_hat;               ///< Оценка трансмиссивности
    double xi_hat;              ///< Оценка избыточного шума (SNU)
    double T_ci_half;           ///< Полуширина ДИ для T (95%)
    double xi_ci_half;          ///< Полуширина ДИ для ξ (95%)
    double sigma2_residual;     ///< Дисперсия остатков
    std::size_t n_samples;
    double slope = 0.0;
    double slope_se = 0.0;
    double slope_lower = 0.0;
    double slope_upper = 0.0;
    double T_lower = 0.0;
    double T_upper = 0.0;
    double xi_lower = 0.0;
    double xi_upper = 0.0;
    double variance_lower = 0.0;
    double variance_upper = 0.0;
    bool identifiable = false;
};

/// MLE-оценка параметров канала по парам (x_A, y_B) методом наименьших квадратов.
class ParameterEstimator {
public:
    explicit ParameterEstimator(const ProtocolConfig& cfg, double confidence_level = 0.95);

    ChannelEstimate estimate(const VectorR& x_alice,
                             const VectorR& y_bob) const;
private:
    ProtocolConfig cfg_;
    double conf_;
};

} // namespace cvqkd