#pragma once
#include "core/types.hpp"

namespace cvqkd {

/// Упрощённая модель информационного согласования (β·I_AB).
class Reconciliation {
public:
    explicit Reconciliation(double beta) : beta_(beta) {}

    struct Result {
        std::size_t reconciled_bits;
        std::size_t disclosed_bits;
        double      efficiency;
    };

    Result run(const VectorR& alice_bits, const VectorR& bob_bits) const;

private:
    double beta_;
};

} // namespace cvqkd