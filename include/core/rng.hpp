#pragma once
#include <cstdint>
#include <random>

namespace cvqkd {

/// Потокобезопасный (локальный) генератор с детерминированным seed.
class RNG {
public:
    explicit RNG(std::uint64_t seed = 0) : engine_(seed) {}

    std::mt19937_64&       engine()       noexcept { return engine_; }
    const std::mt19937_64& engine() const noexcept { return engine_; }

    double   uniform01()        { return u01_(engine_); }
    int      uniform_int(int a, int b) {
        std::uniform_int_distribution<int> d(a, b); return d(engine_);
    }
    double   normal(double mean = 0.0, double sigma = 1.0) {
        std::normal_distribution<double> d(mean, sigma); return d(engine_);
    }

private:
    std::mt19937_64                      engine_;
    std::uniform_real_distribution<double> u01_{0.0, 1.0};
};

} // namespace cvqkd