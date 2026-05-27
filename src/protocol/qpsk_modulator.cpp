#include "protocol/qpsk_modulator.hpp"
#include <cmath>

namespace cvqkd {

QPSKModulator::QPSKModulator(double alpha, std::uint64_t seed)
    : alpha_(alpha), rng_(seed) {}

AliceSignal QPSKModulator::generate(std::size_t N) {
    AliceSignal s;
    s.keys.reserve(N);
    s.amplitudes.reserve(N);

    constexpr Complex I(0.0, 1.0);
    for (std::size_t i = 0; i < N; ++i) {
        const std::uint8_t k = static_cast<std::uint8_t>(rng_.uniform_int(0, 3));
        s.keys.push_back(k);
        // α · i^k : i⁰=1, i¹=i, i²=-1, i³=-i
        Complex amp = alpha_;
        switch (k) {
            case 0: break;
            case 1: amp =  I * alpha_; break;
            case 2: amp =     -alpha_; break;
            case 3: amp = -I * alpha_; break;
        }
        s.amplitudes.push_back(amp);
    }
    return s;
}

} // namespace cvqkd