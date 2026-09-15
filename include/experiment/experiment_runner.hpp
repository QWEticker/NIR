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

    void run_one_point(ProtocolConfig pc, std::size_t point, std::size_t repeat);
};

} // namespace cvqkd
