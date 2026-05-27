#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
#include "json.hpp"  // Используем прямой путь, так как json.hpp лежит в extern/nlohmann/


namespace cvqkd {

/// Конфигурация протокола и физических параметров системы.
struct ProtocolConfig {
    // --- Источник ---
    double alpha            = 0.5;     ///< Амплитуда когерентного состояния
    // --- Канал ---
    double T                = 0.1;     ///< Трансмиссивность канала [0,1]
    double xi               = 0.01;    ///< Избыточный шум (SNU)
    double distance_km      = 20.0;    ///< Длина линии (для T = 10^(-αL/1S0))
    // --- Приёмник ---
    double eta              = 0.6;     ///< Квантовая эффективность детектора
    double v_el             = 0.05;    ///< Электронный шум детектора (SNU)
    double saturation_level = 10.0;    ///< Порог насыщения АЦП (SNU)
    // --- Постобработка ---
    double beta             = 0.95;    ///< Эффективность reconciliation
    // --- Статистика ---
    std::size_t N           = 1'000'000; ///< Число импульсов
    std::uint64_t seed      = 42;      ///< Seed ГПСЧ

    /// Вычислить T из расстояния, если оно задано.
    void recompute_T_from_distance() noexcept;
};

/// Конфигурация серии экспериментов.
struct ExperimentConfig {
    std::string               name;
    ProtocolConfig            base;
    std::vector<double>       distances_km;       ///< sweep по дистанциям
    std::vector<std::string>  attacks;            ///< {"none","saturation","lo"}
    std::size_t               runs_per_point = 5; ///< число повторов

    static ExperimentConfig from_json_file(const std::string& path);
};

} // namespace cvqkd