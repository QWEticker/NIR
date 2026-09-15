#include "core/config.hpp"
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>

namespace cvqkd {
namespace {
using json = nlohmann::json;

void check_keys(const json& value, const std::set<std::string>& allowed) {
    if (!value.is_object()) throw std::invalid_argument("configuration must be an object");
    for (const auto& entry : value.items())
        if (allowed.count(entry.key()) == 0 && entry.key().find('_') != 0)
            throw std::invalid_argument("unknown configuration parameter: " + entry.key());
}

std::uint64_t unsigned_value(const json& value) {
    if (!value.is_number_integer() ||
        (!value.is_number_unsigned() && value.get<std::int64_t>() < 0))
        throw std::invalid_argument("count or seed must be a nonnegative integer");
    return value.get<std::uint64_t>();
}
} // namespace

std::string source_revision() { return CVQKD_SOURCE_REVISION; }
std::string source_fingerprint() { return CVQKD_SOURCE_FINGERPRINT; }

void ProtocolConfig::recompute_T_from_distance() noexcept {
    T = std::pow(10.0, -0.02 * distance_km);
}

void ProtocolConfig::validate() const {
    if (!std::isfinite(alpha) || alpha <= 0.0 || !std::isfinite(2 * alpha * alpha) ||
        !std::isfinite(T) || T < 0.0 || T > 1.0 || !std::isfinite(xi) || xi < 0.0 ||
        !std::isfinite(distance_km) || distance_km < 0.0 ||
        !std::isfinite(eta) || eta <= 0.0 || eta > 1.0 ||
        !std::isfinite(v_el) || v_el < 0.0 ||
        std::isnan(saturation_level) || saturation_level <= 0.0 ||
        !std::isfinite(beta) || beta < 0.0 || beta > 1.0 ||
        !std::isfinite(confidence_level) || confidence_level <= 0.0 || confidence_level >= 1.0 ||
        N < 3 || N > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max() / 2))
        throw std::invalid_argument("invalid protocol parameters; see docs/article_models.md");
}

void ExperimentConfig::validate() const {
    base.validate();
    if (name.empty() || runs_per_point == 0 || attack_params_json.size() > attacks.size())
        throw std::invalid_argument("experiment requires a name, positive repeats and matching attacks");
    for (double distance : distances_km)
        if (!std::isfinite(distance) || distance < 0.0)
            throw std::invalid_argument("distances must be finite and nonnegative");
    if (distances_km.size() > std::numeric_limits<std::size_t>::max() / runs_per_point)
        throw std::overflow_error("experiment point count overflow");
}

ExperimentConfig ExperimentConfig::from_json_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open configuration: " + path);
    json j;
    input >> j;
    check_keys(j, {"name", "base", "distances_km", "attacks", "runs_per_point"});
    const auto& b = j.at("base");
    check_keys(b, {"alpha", "T", "xi", "distance_km", "eta", "v_el", "saturation_level",
                   "beta", "N", "seed", "confidence_level"});
    ExperimentConfig cfg;
    cfg.name = j.at("name").get<std::string>();
    auto& pc = cfg.base;
    pc.alpha = b.value("alpha", pc.alpha);
    pc.xi = b.value("xi", pc.xi);
    pc.eta = b.value("eta", pc.eta);
    pc.v_el = b.value("v_el", pc.v_el);
    pc.beta = b.value("beta", pc.beta);
    pc.confidence_level = b.value("confidence_level", pc.confidence_level);
    if (b.contains("saturation_level"))
        pc.saturation_level = b.at("saturation_level").is_null() ?
            std::numeric_limits<double>::infinity() : b.at("saturation_level").get<double>();
    if (b.contains("N")) pc.N = static_cast<std::size_t>(unsigned_value(b.at("N")));
    if (b.contains("seed")) pc.seed = unsigned_value(b.at("seed"));
    if (b.contains("distance_km")) {
        pc.distance_km = b.at("distance_km").get<double>();
        pc.recompute_T_from_distance();
    }
    pc.T = b.value("T", pc.T);
    if (j.contains("distances_km")) cfg.distances_km = j.at("distances_km").get<std::vector<double>>();
    if (j.contains("runs_per_point"))
        cfg.runs_per_point = static_cast<std::size_t>(unsigned_value(j.at("runs_per_point")));
    if (j.contains("attacks")) {
        if (!j.at("attacks").is_array()) throw std::invalid_argument("attacks must be an array");
        for (const auto& attack : j.at("attacks")) {
            if (attack.is_string()) {
                cfg.attacks.push_back(attack.get<std::string>());
                cfg.attack_params_json.emplace_back();
            } else if (attack.is_object()) {
                cfg.attacks.push_back(attack.at("name").get<std::string>());
                cfg.attack_params_json.push_back(attack.dump());
            } else {
                throw std::invalid_argument("attack must be a name or an object with a name");
            }
        }
    }
    cfg.validate();
    return cfg;
}

} // namespace cvqkd
