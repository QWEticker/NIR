#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

#include "nlohmann/json.hpp"
#include "XiMaxSolver.h"
#include "HolevoCalculator.h"
#include "FiniteKeyAnalyzer.h"
#include "AttackBase.h"
#include "attacks/AttackRegistry.cpp"
#include "ConsoleUtils.h"
#include "ChannelUtils.h"

using json = nlohmann::json;
using namespace nir;

static double computeKeyRate(double L, double alpha, double V_A, double eta, double v_el, double beta, uint64_t n, double eps_total, double xi) {
    // Compute v_det
    HeterodyneReceiverDetailed recv(eta, v_el);
    double v_det = recv.computeVdet();
    double T = transmissivityFromDistance(L, alpha);

    // Mutual information (Gaussian approx)
    double signal = T * V_A;
    double noise = 1.0 + T * xi + v_det;
    double snr = (noise > 0.0) ? signal / noise : 0.0;
    double Iab = 0.5 * std::log2(1.0 + snr);

    double chi = 0.0;
    // Build CM and compute Holevo
    Eigen::Matrix4d Vbe = HolevoCalculator::buildEntanglingClonerCM(T, V_A, xi);
    chi = HolevoCalculator::computeHolevoFromCM(Vbe);

    double delta = FiniteKeyAnalyzer::finiteSizeDelta(n, eps_total);
    double K = beta * Iab - chi - delta;
    return K;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    initConsoleUtf8();
#endif
    if (argc < 3) {
        std::cout << "Usage: cvqkd_sim --config <config.json> --output <out.csv>" << std::endl;
        return 1;
    }

    std::string config_path;
    std::string out_path = "results.csv";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--config" && i+1 < argc) { config_path = argv[++i]; }
        else if (a == "--output" && i+1 < argc) { out_path = argv[++i]; }
    }
    if (config_path.empty()) {
        std::cerr << "FATAL: Cannot open config file: missing --config" << std::endl;
        return 2;
    }

    std::ifstream ifs(config_path);
    if (!ifs) {
        std::cerr << "FATAL: Cannot open config file: " << config_path << std::endl;
        return 3;
    }

    json cfg;
    try { ifs >> cfg; } catch (std::exception &e) {
        std::cerr << "FATAL: Invalid JSON config: " << e.what() << std::endl;
        return 4;
    }

    json base = cfg.value("base", json::object());
    double alpha = base.value("alpha", 0.2);
    double xi = base.value("xi", 0.01);
    double eta = base.value("eta", 0.8);
    double v_el = base.value("v_el", 0.005);
    double beta = base.value("beta", 0.95);
    double sat_level = base.value("saturation_level", 10.0);
    uint64_t N = base.value("N", 1000000);
    int seed = base.value("seed", 42);
    double V_A = base.value("V_A", 0.5);
    double eps_total = base.value("eps_total", 1e-9);

    std::vector<double> distances;
    if (cfg.contains("distances_km") && cfg["distances_km"].is_array()) {
        for (auto &d : cfg["distances_km"]) distances.push_back(d.get<double>());
    }
    else if (cfg.contains("distance_km")) distances.push_back(cfg["distance_km"].get<double>());
    else distances = {5.0};

    std::vector<json> attacks;
    if (cfg.contains("attacks") && cfg["attacks"].is_array()) {
        for (auto &a : cfg["attacks"]) attacks.push_back(a);
    } else if (cfg.contains("attack")) attacks.push_back(cfg["attack"]);
    else attacks.push_back(json({{"name","none"},{"param",0.0}}));

    int runs_per_point = cfg.value("runs_per_point", 1);

    std::ofstream ofs(out_path);
    if (!ofs) {
        std::cerr << "FATAL: Cannot open output file: " << out_path << std::endl;
        return 5;
    }

    // CSV header
    ofs << "experiment,distance_km,attack_name,attack_param,xi_attack,xi_total,xi_max,key_rate_per_symbol,secure" << std::endl;
    std::string experiment = cfg.value("name","experiment");

    for (double d : distances) {
        for (auto &attack_j : attacks) {
            std::string aname = "none";
            double aparam = 0.0;
            if (attack_j.is_object()) {
                aname = attack_j.value("name", std::string("none"));
                aparam = attack_j.value("param", 0.0);
            } else if (attack_j.is_string()) {
                aname = attack_j.get<std::string>();
            }

            // create attack
            std::shared_ptr<AttackBase> atk = AttackRegistry::createByName(aname, aparam);
            double xi_attack = 0.0;
            if (atk) xi_attack = atk->inducedExcessNoise();
            double xi_total = xi + xi_attack;

            // compute xi_max
            XiMaxResult r = XiMaxSolver::solve(d, alpha, V_A, eta, v_el, beta, N, eps_total);
            double xi_max = r.success ? r.xi_max : 0.0;

            // compute key rate at xi_total
            double K = computeKeyRate(d, alpha, V_A, eta, v_el, beta, N, eps_total, xi_total);
            bool secure = (K > 0.0 && xi_total <= xi_max);

            ofs << experiment << "," << d << "," << aname << "," << aparam << "," << xi_attack << "," << xi_total << "," << xi_max << "," << K << "," << (secure?"YES":"NO") << std::endl;
        }
    }

    ofs.close();
    std::cout << "Results written to " << out_path << std::endl;
    return 0;
}
