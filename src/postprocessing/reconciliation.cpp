#include "postprocessing/reconciliation.hpp"
#include <cmath>

namespace cvqkd {

Reconciliation::Result
Reconciliation::run(const VectorR& a, const VectorR& b) const {
    // Модель: эффективность = β; число общих бит ~ N·β·I_AB
    const Eigen::Index n = std::min(a.size(), b.size());
    std::size_t matching = 0;
    for (Eigen::Index i = 0; i < n; ++i) {
        if ((a(i) > 0) == (b(i) > 0)) ++matching;
    }
    const double ber = 1.0 - static_cast<double>(matching) / n;
    const double I   = 1.0 + ber * std::log2(ber + 1e-300)
                           + (1-ber) * std::log2(1-ber + 1e-300);
    Result r{};
    r.reconciled_bits = static_cast<std::size_t>(n * I);
    r.disclosed_bits  = static_cast<std::size_t>(n * (1.0 - beta_ * I));
    r.efficiency      = beta_;
    return r;
}

} // namespace cvqkd