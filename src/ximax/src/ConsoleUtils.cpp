#include "ConsoleUtils.h"
#include <string>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace nir {

#ifdef _WIN32
void initConsoleUtf8() {
    // Set console output code page to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // Enable virtual terminal processing for ANSI escape sequences (optional)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
}
#endif

std::string formatDouble(double v, int precision) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(precision) << v;
    return oss.str();
}

} // namespace nir
