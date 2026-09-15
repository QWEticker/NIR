#include "protocol/quantum_channel.hpp"
#include <cmath>
#include <random> // Обязательно!
#include <stdexcept>

namespace cvqkd {

QuantumChannel::QuantumChannel(const ProtocolConfig& cfg)
    : cfg_(cfg), rng_(cfg.seed ^ 0xC0FFEEull) {
    if (!std::isfinite(cfg.T) || cfg.T < 0.0 || cfg.T > 1.0 ||
        !std::isfinite(cfg.xi) || cfg.xi < 0.0)
        throw std::invalid_argument("channel requires T in [0,1] and xi >= 0");
}

std::vector<Complex> QuantumChannel::transmit(const std::vector<Complex>& alice) const {
    std::vector<Complex> bob;
    bob.reserve(alice.size());

    const double sqrtT = std::sqrt(cfg_.T);
    const double sigma = std::sqrt(cfg_.T * cfg_.xi / 4.0);

    // Создаем объект распределения. Он НЕ является членом класса,
    // чтобы избежать проблем с const-correctness и состоянием.
    std::normal_distribution<double> dist(0.0, 1.0);

    for (const auto& a : alice) {
        // Правильный вызов: distribution(generator)
        if (!std::isfinite(a.real()) || !std::isfinite(a.imag()))
            throw std::invalid_argument("channel input must be finite");
        const double nx = sigma * dist(rng_.engine());
        const double np = sigma * dist(rng_.engine());
        
        bob.push_back(sqrtT * a + Complex(nx, np));
    }
    return bob;
}

} // namespace cvqkd