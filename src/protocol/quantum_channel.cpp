#include "protocol/quantum_channel.hpp"
#include <cmath>
#include <random> // Обязательно!

namespace cvqkd {

QuantumChannel::QuantumChannel(const ProtocolConfig& cfg)
    : cfg_(cfg), rng_(cfg.seed ^ 0xC0FFEEull) {}

std::vector<Complex> QuantumChannel::transmit(const std::vector<Complex>& alice) const {
    std::vector<Complex> bob;
    bob.reserve(alice.size());

    const double sqrtT = std::sqrt(cfg_.T);
    const double sigma = std::sqrt(cfg_.T * cfg_.xi / 2.0);

    // Создаем объект распределения. Он НЕ является членом класса,
    // чтобы избежать проблем с const-correctness и состоянием.
    std::normal_distribution<double> dist(0.0, sigma);

    for (const auto& a : alice) {
        // Правильный вызов: distribution(generator)
        const double nx = dist(rng_.engine());
        const double np = dist(rng_.engine());
        
        bob.push_back(sqrtT * a + Complex(nx, np));
    }
    return bob;
}

} // namespace cvqkd