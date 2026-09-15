#pragma once
#include "core/types.hpp"
#include "core/config.hpp"
#include "core/rng.hpp"

namespace cvqkd {

/// Гетеродинный детектор: измеряет (X, P) с квантовой эффективностью η
/// и электронным шумом v_el.
class HeterodyneDetector {
public:
    explicit HeterodyneDetector(const ProtocolConfig& cfg);

    Measurement measure(const std::vector<Complex>& incoming);

    /// Per-component variance of the uncalibrated amplitude readout.
    double detector_noise_variance() const noexcept;
    std::size_t clipped_components() const noexcept { return clipped_; }

private:
    ProtocolConfig cfg_;
    mutable RNG    rng_;  // ← Добавлено mutable
    std::size_t clipped_ = 0;

};

} // namespace cvqkd