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

    /// Суммарная дисперсия шума детектора, приведённая ко входу (SNU).
    double detector_noise_variance() const noexcept;

private:
    ProtocolConfig cfg_;
    mutable RNG    rng_;  // ← Добавлено mutable

};

} // namespace cvqkd