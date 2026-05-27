#include "experiment/statistics_logger.hpp"
#include <filesystem> // ← Добавить в начало

namespace cvqkd {

StatisticsLogger::StatisticsLogger(const std::string& p) {
    namespace fs = std::filesystem;
    
    // Создаём все промежуточные папки, если их нет
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
            "T_hat,xi_hat,T_ci,xi_ci,"
            "I_AB,chi_BE,K_asymp,K_beta,secure,elapsed_ms\n";
}

void StatisticsLogger::log(const std::string& sc, double d,
                           double T, double xi,
                           const ChannelEstimate& e,
                           const SecurityMetrics& m,
                           double ms) {
    out_ << sc << ',' << d << ',' << T << ',' << xi << ','
         << e.T_hat << ',' << e.xi_hat << ','
         << e.T_ci_half << ',' << e.xi_ci_half << ','
         << m.I_AB << ',' << m.chi_BE << ','
         << m.K_asymptotic << ',' << m.K_beta << ','
         << (m.secure() ? 1 : 0) << ',' << ms << '\n';
    out_.flush();
}

} // namespace cvqkd