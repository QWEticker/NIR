#include "attacks/collective_attack.hpp"
#include <cmath>

namespace cvqkd {

CollectiveAttack::CollectiveAttack(double coupling_strength,
                                   double excess_noise,
                                   std::uint64_t seed)
    : coupling_(coupling_strength)
    , excess_noise_(excess_noise)
    , rng_(seed)
    , noise_(0.0, 1.0) {}

void CollectiveAttack::apply(Measurement& m) const {
    // Моделирование коллективной атаки:
    // 1. Ева взаимодействует с сигналом через beam-splitter с коэффициентом coupling_
    // 2. Добавляется избыточный шум от взаимодействия
    // 3. Сигнал Боба ослабляется и зашумляется
    
    const double vacuum_noise = 1.0; // SNU
    const double transmission = std::sqrt(1.0 - coupling_ * coupling_);
    const double coupling = coupling_;
    
    // Шум от взаимодействия (избыточный шум атаки)
    const double attack_noise = excess_noise_ * vacuum_noise;
    
    for (Eigen::Index i = 0; i < m.X.size(); ++i) {
        // Сигнал проходит через beam-splitter взаимодействия
        // Часть сигнала уходит к Еве, часть остаётся у Боба
        const double bob_X = m.X(i) * transmission + 
                             noise_(rng_) * coupling * vacuum_noise;
        const double bob_P = m.P(i) * transmission + 
                             noise_(rng_) * coupling * vacuum_noise;
        
        // Добавляем избыточный шум от атаки
        m.X(i) = bob_X + noise_(rng_) * attack_noise;
        m.P(i) = bob_P + noise_(rng_) * attack_noise;
    }
}

} // namespace cvqkd
