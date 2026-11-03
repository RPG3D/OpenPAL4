#include "pal4/time_formatter.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main() {
    try {
        const std::vector<std::pair<std::int32_t, std::string>> cases{
            {0, "0: 00: 00"},
            {1, "0: 00: 01"},
            {9, "0: 00: 09"},
            {10, "0: 00: 10"},
            {59, "0: 00: 59"},
            {60, "0: 01: 00"},
            {599, "0: 09: 59"},
            {600, "0: 10: 00"},
            {3599, "0: 59: 59"},
            {3600, "1: 00: 00"},
            {86399, "23: 59: 59"},
            {-1, "0: 00: 0-1"},
            {-60, "0: 0-1: 00"},
            {-3600, "-1: 00: 00"},
            {-3700, "-1: 0-1: 0-40"},
            {std::numeric_limits<std::int32_t>::max(),
             "596523: 14: 07"},
            {std::numeric_limits<std::int32_t>::min(),
             "-596523: 0-14: 0-8"},
        };
        for (const auto& [seconds, expected] : cases) {
            Check(pal4::TimeFormatter::FormatSeconds(seconds) == expected,
                  "time formatting differs from IDA's signed output");
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
