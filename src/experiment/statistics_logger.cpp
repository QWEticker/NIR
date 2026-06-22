#include "experiment/statistics_logger.hpp"
#include <filesystem>

namespace cvqkd {

StatisticsLogger::StatisticsLogger(const std::string& p) {
    namespace fs = std::filesystem;
    if (auto dir = fs::path(p).parent_path(); !dir.empty() && !fs::exists(dir)) {
        fs::create_directories(dir);
    }

    out_.open(p);
    if (!out_) {
        throw std::runtime_error("Cannot open log file: " + p);
    }
    write_header();
}

void StatisticsLogger::write_header() {
    out_ << "scenario,distance_km,T_true,xi_true,"
            "T_hat,xi_hat,xi_eff,T_ci,xi_ci,"
            "I_AB,chi_BE,K_asymp,K_beta,secure,elapsed_ms,attack_name,attack_params\n";
}

void StatisticsLogger::log(const std::string& sc, double d,
                           double T, double xi,
                           const ChannelEstimate& e,
                           const SecurityMetrics& m,
                           double ms,
                           const std::string &attack_name,
                           const std::string &attack_params) {
    out_ << sc << ',' << d << ',' << T << ',' << xi << ','
         << e.T_hat << ',' << e.xi_hat << ',' << m.xi_eff << ','
         << e.T_ci_half << ',' << e.xi_ci_half << ','
         << m.I_AB << ',' << m.chi_BE << ','
         << m.K_asymptotic << ',' << m.K_beta << ','
         << (m.secure() ? 1 : 0) << ',' << ms << ','
         << '"' << attack_name << '"' << ',' << '"' << attack_params << '"' << '\n';
    out_.flush();
}

} // namespace cvqkd
