#include "core/config.hpp"
#include "json.hpp"  // Используем прямой путь, так как json.hpp лежит в extern/nlohmann/
#include <fstream>
#include <stdexcept>

namespace cvqkd {

void ProtocolConfig::recompute_T_from_distance() noexcept {
    if (distance_km > 0.0) {
        // T = 10^(-α·L/10), где α = 0.2 дБ/км для SMF-28 @ 1550 нм
        T = std::pow(10.0, -0.2 * distance_km / 10.0);
    }
}

// Вспомогательная функция для парсинга объекта ProtocolConfig из JSON
static void from_json(const nlohmann::json& j, ProtocolConfig& cfg) {
    j.at("alpha").get_to(cfg.alpha);
    j.at("xi").get_to(cfg.xi);
    j.at("eta").get_to(cfg.eta);
    j.at("v_el").get_to(cfg.v_el);
    j.at("beta").get_to(cfg.beta);
    j.at("saturation_level").get_to(cfg.saturation_level);
    j.at("N").get_to(cfg.N);
    j.at("seed").get_to(cfg.seed);
    
    // Опциональные поля
    if (j.contains("distance_km")) {
        j.at("distance_km").get_to(cfg.distance_km);
        cfg.recompute_T_from_distance();
    }
    if (j.contains("T")) {
        j.at("T").get_to(cfg.T);
    }
}

// Вспомогательная функция для парсинга ExperimentConfig
static void from_json(const nlohmann::json& j, ExperimentConfig& exp) {
    j.at("name").get_to(exp.name);
    j.at("base").get_to(exp.base);
    
    if (j.contains("distances_km")) {
        j.at("distances_km").get_to(exp.distances_km);
    }
    if (j.contains("attacks")) {
        j.at("attacks").get_to(exp.attacks);
    }
    if (j.contains("runs_per_point")) {
        j.at("runs_per_point").get_to(exp.runs_per_point);
    }
}

ExperimentConfig ExperimentConfig::from_json_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open config file: " + path);
    }
    
    nlohmann::json j;
    f >> j;
    
    ExperimentConfig cfg;
    from_json(j, cfg);
    return cfg;
}

} // namespace cvqkd