#include "experiment/statistics_logger.hpp"
#include <filesystem>
#include <iomanip>
#include <stdexcept>

namespace cvqkd {
namespace {
std::string csv_field(const std::string& text) {
    std::string escaped = "\"";
    for (char ch : text) {
        if (ch == '"') escaped += '"';
        escaped += ch;
    }
    return escaped + '"';
}
} // namespace

StatisticsLogger::StatisticsLogger(const std::string& path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    out_.open(path, std::ios::out | std::ios::binary);
    if (!out_) throw std::runtime_error("cannot open output: " + path);
    out_ << std::setprecision(17);
}

void StatisticsLogger::write_header() {
    out_ << "scenario,distance_km,T_true,xi_true,T_hat,xi_hat,xi_eff,T_ci,xi_ci,"
            "I_AB,chi_BE,K_asymp,K_beta,secure,elapsed_ms,attack_name,attack_params,model,details\n";
}

void StatisticsLogger::log(const std::string& scenario, double distance, double T, double xi,
                           const ChannelEstimate& e, const SecurityMetrics& m, double elapsed,
                           const std::string& attack, const std::string& params,
                           const nlohmann::json& details) {
    out_ << csv_field(scenario) << ',' << distance << ',' << T << ',' << xi << ','
         << e.T_hat << ',' << e.xi_hat << ',' << m.xi_eff << ',' << e.T_ci_half << ','
         << e.xi_ci_half << ',' << m.I_AB << ',' << m.chi_BE << ',' << m.K_asymptotic << ','
         << m.K_beta << ',' << m.secure() << ',' << elapsed << ',' << csv_field(attack) << ','
         << csv_field(params) << ',' << csv_field(m.model) << ',' << csv_field(details.dump()) << '\n';
    out_.flush();
    if (!out_) throw std::runtime_error("failed to write experiment results");
}

} // namespace cvqkd
