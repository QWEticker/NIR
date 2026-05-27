#pragma once

namespace cvqkd::constants {

/// Приведённая постоянная Планка в SNU (shot-noise units).
constexpr double HBAR           = 1.0;
/// Дисперсия вакуумного шума (1 SNU по определению).
constexpr double VAC_VARIANCE   = 1.0;
/// Затухание в стандартном одномодовом волокне SMF-28, дБ/км @ 1550 нм.
constexpr double FIBER_LOSS_DB_PER_KM = 0.2;

} // namespace cvqkd::constantsSSS