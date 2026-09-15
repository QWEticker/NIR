#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
#include "json.hpp"  // Используем прямой путь, так как json.hpp лежит в extern/nlohmann/


namespace cvqkd {
std::string source_revision();
std::string source_fingerprint();

/// Конфигурация протокола и физических параметров системы.
struct ProtocolConfig {
    // --- Источник ---
    double alpha            = 0.5;     ///< Амплитуда когерентного состояния
    // --- Канал ---
    double T                = 0.3981071705534972; ///< Power transmission at 20 km
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
    double confidence_level = 0.95;

    /// Вычислить T из расстояния, если оно задано.
    void recompute_T_from_distance() noexcept;
    void validate() const;
};

/// Конфигурация серии экспериментов.
struct ExperimentConfig {
    std::string               name;
    ProtocolConfig            base;
    std::vector<double>       distances_km;       ///< sweep по дистанциям
    std::vector<std::string>  attacks;            ///< {"none","saturation","lo"}
    // Optionally the config may contain attack descriptions as full JSON objects —
    // we'll store raw JSON strings to keep backward compatibility with simple string list.
    std::vector<std::string>  attack_params_json;
    std::size_t               runs_per_point = 5; ///< число повторов

    static ExperimentConfig from_json_file(const std::string& path);
    void validate() const;
};

} // namespace cvqkd
