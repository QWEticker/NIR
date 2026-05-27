#include "experiment/experiment_runner.hpp"
#include "experiment/statistics_logger.hpp"
#include "attacks/saturation_attack.hpp"
#include "attacks/lo_manipulation_attack.hpp"
#include "attacks/intercept_resend_attack.hpp"
#include "attacks/collective_attack.hpp"
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

using namespace cvqkd;

int main(int argc, char** argv) {
    std::string cfg_path = "experiments/nominal.json";
    std::string out_csv  = "results/experiment.csv";

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc)
            cfg_path = argv[++i];
        else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc)
            out_csv = argv[++i];
    }

    try {
        auto cfg = ExperimentConfig::from_json_file(cfg_path);
        auto log = std::make_shared<StatisticsLogger>(out_csv);
        ExperimentRunner runner(cfg, log);

        // Пример подключения атак по имени (из конфига):
        for (const auto& atk : cfg.attacks) {
            if (atk == "saturation")
                runner.attach_attack(
                    std::make_unique<SaturationAttack>(cfg.base.saturation_level, 2.0));
            else if (atk == "lo")
                runner.attach_attack(std::make_unique<LOManipulationAttack>(0.7));
            else if (atk == "intercept_resend")
                runner.attach_attack(std::make_unique<InterceptResendAttack>(0.8, 1.0, cfg.base.seed + 1000));
            else if (atk == "collective")
                runner.attach_attack(std::make_unique<CollectiveAttack>(0.3, 0.05, cfg.base.seed + 2000));
        }

        runner.run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << '\n';
        return 1;
    }
    return 0;
}