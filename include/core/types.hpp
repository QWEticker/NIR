#pragma once
#include <Eigen/Core>
#include <Eigen/Dense>
#include <complex>
#include <vector>

namespace cvqkd {

using Complex  = std::complex<double>;
using VectorCd = Eigen::VectorXcd;
using MatrixCd = Eigen::MatrixXcd;
using VectorR  = Eigen::VectorXd;
using MatrixR  = Eigen::MatrixXd;

/// Результат гетеродинного измерения (обе квадратуры).
struct Measurement {
    VectorR X; ///< Квадратура амплитуды
    VectorR P; ///< Квадратура фазы
    std::size_t size() const noexcept { return static_cast<std::size_t>(X.size()); }
};

/// «Чистый» сигнал Алисы: ключи и соответствующие комплексные амплитуды.
struct AliceSignal {
    std::vector<std::uint8_t> keys;  ///< 0..3 для QPSK
    std::vector<Complex> amplitudes; ///< α·i^k
};

} // namespace cvqkd