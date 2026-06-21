#pragma once
#include "attacks/i_attack.hpp"
#include "attacks/attack_model.hpp"
#include "core/config.hpp"
#include "experiment/statistics_logger.hpp"
#include <memory>

namespace cvqkd {

class ExperimentRunner {
  public:
    explicit ExperimentRunner(ExperimentConfig cfg,
                              std::shared_ptr<StatisticsLogger> logger);

    void run();

    /// Подключить атаку (может быть несколько, применяются последовательно).
    void attach_attack(std::unique_ptr<IAttack> atk);

  private:
    ExperimentConfig cfg_;
    std::shared_ptr<StatisticsLogger> logger_;
    std::vector<std::unique_ptr<IAttack>> attacks_;

    // Analytical models corresponding to attacks (for security analysis)
    std::vector<std::unique_ptr<cvqkd::IAttackModel>> attack_models_;
    std::vector<std::string> attack_names_;
    std::vector<std::string> attack_params_;

    void run_one_point(double distance_km, const std::string &scenario);
};

} // namespace cvqkd
