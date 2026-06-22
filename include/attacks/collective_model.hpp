#pragma once
#include "attacks/attack_model.hpp"

namespace cvqkd {

// Аналитическая модель коллективной (гауссовой) атаки. Безопасность когерентного
// CV-QKD как раз ограничивается оптимальной коллективной атакой, поэтому её эффект
// сводится к добавочному избыточному шуму канала, который входит в стандартную
// границу Холево SecurityAnalyzer.
class CollectiveModel : public IAttackModel {
public:
    explicit CollectiveModel(double coupling = 0.3, double excess_noise = 0.05);
    AttackResult computeEffect(double T, double V_A, double xi_in) const override;
    std::string name() const override { return "collective"; }
private:
    double coupling_;
    double excess_noise_;
};

} // namespace cvqkd
