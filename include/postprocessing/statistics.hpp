#pragma once
#include "core/types.hpp"
#include <array>
#include <cstdint>
#include <limits>

namespace cvqkd {

double student_quantile(double probability, double degrees_of_freedom);
double chi_square_quantile(double probability, double degrees_of_freedom);
double qpsk_awgn_mutual_information(double alpha, double gain, double component_variance);

struct SymbolStatistics {
    std::array<std::array<std::uint64_t, 5>, 4> counts{};
    std::uint64_t total = 0;
    std::uint64_t observed = 0;
    double mutual_information = 0.0;
    double symbol_error_rate = std::numeric_limits<double>::quiet_NaN();
    double bit_error_rate = std::numeric_limits<double>::quiet_NaN();
};

std::size_t qpsk_decision(double x, double p);
SymbolStatistics symbol_statistics(const std::vector<std::uint8_t>& alice,
                                   const Measurement& receiver,
                                   const std::vector<std::uint8_t>& observed = {});

} // namespace cvqkd
