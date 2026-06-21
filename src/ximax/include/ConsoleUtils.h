#pragma once

namespace nir {

#ifdef _WIN32
void initConsoleUtf8();
#endif

// Format double to fixed string with 6 decimal places
std::string formatDouble(double v, int precision = 6);

} // namespace nir
