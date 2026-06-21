#pragma once

namespace nir {

// Compute channel transmissivity from fiber distance L (km) and attenuation alpha (dB/km)
inline double transmissivityFromDistance(double L, double alpha_db_per_km) {
    return std::pow(10.0, -alpha_db_per_km * L / 10.0);
}

} // namespace nir
