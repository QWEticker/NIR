#pragma once
#include "postprocessing/parameter_estimator.hpp"
#include "postprocessing/security_analyzer.hpp"
#include <fstream>
#include <string>
#include <vector>

namespace cvqkd {

/// CSV-логгер результатов экспериментов.
class StatisticsLogger {
  public:
    explicit StatisticsLogger(const std::string &csv_path);

    void write_header();
    void log(const std::string &scenario, double distance_km, double T_true,
             double xi_true, const ChannelEstimate &est, const SecurityMetrics &metrics,
             double elapsed_ms, const std::string &attack_name = "", const std::string &attack_params = "");

  private:
    std::ofstream out_;
};

} // namespace cvqkd
