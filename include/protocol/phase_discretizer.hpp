#pragma once
#include "core/types.hpp"

namespace cvqkd {

/// Дискретизация фазового угла квадратур (квантование arg(X+iP)).
class PhaseDiscretizer {
public:
    explicit PhaseDiscretizer(std::size_t bins = 4) : bins_(bins) {}

    /// Возвращает индекс бина [0, bins) для каждого измерения.
    std::vector<std::size_t> discretize(const Measurement& m) const;

private:
    std::size_t bins_;
};

} // namespace cvqkd