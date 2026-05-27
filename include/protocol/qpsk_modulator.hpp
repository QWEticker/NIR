#pragma once
#include "core/types.hpp"
#include "core/rng.hpp"

namespace cvqkd {

/// Генератор QPSK-модулированных когерентных состояний
/// |α⟩, |iα⟩, |-α⟩, |-iα⟩.
class QPSKModulator {
public:
    explicit QPSKModulator(double alpha, std::uint64_t seed = 42);

    AliceSignal generate(std::size_t N);

    double alpha() const noexcept { return alpha_; }
    /// Дисперсия модуляции V_A = α² (в SNU) для QPSK.
    double modulation_variance() const noexcept { return alpha_ * alpha_; }

private:
    double alpha_;
    RNG    rng_;
};

} // namespace cvqkd