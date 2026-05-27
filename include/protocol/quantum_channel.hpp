#pragma once
#include "core/types.hpp"
#include "core/config.hpp"
#include "core/rng.hpp"
#include <vector>
#include <random>

namespace cvqkd {

/// Модель канала с потерями T и приведённым избыточным шумом ξ (SNU).
class QuantumChannel {
public:
    explicit QuantumChannel(const ProtocolConfig& cfg);

    /// Принимает амплитуды Алисы, возвращает амплитуды на входе детектора Боба.
    std::vector<Complex> transmit(const std::vector<Complex>& alice) const;

    double T()  const noexcept { return cfg_.T; }
    double xi() const noexcept { return cfg_.xi; }

private:
    ProtocolConfig cfg_;
    mutable RNG    rng_;  // ← Добавлено mutable

};

} // namespace cvqkd