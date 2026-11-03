#include "pal4/trade_window_percentage_formatter.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

int main() {
    struct Case {
        std::int32_t input;
        std::string_view expected;
    };
    constexpr std::array cases{
        Case{-1, "0"}, Case{0, "0"}, Case{1, "1"},
        Case{9, "9"}, Case{10, "10"}, Case{11, "101"},
        Case{23, "203"}, Case{42, "402"}, Case{90, "90"},
        Case{99, "909"}, Case{100, "0"}, Case{101, "0"}};
    try {
        for (const auto& test : cases)
            if (pal4::TradeWindowPercentageFormatter::Format(test.input) !=
                test.expected)
                throw std::runtime_error("format branch or boundary");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
