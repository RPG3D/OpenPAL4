#include "pal4/trade_window_percentage_formatter.h"

namespace pal4 {

std::string TradeWindowPercentageFormatter::Format(std::int32_t value) {
    if (value <= 0 || value >= 100) return "0";
    const auto tens = value / 10;
    const auto ones = value % 10;
    if (tens <= 0) return std::to_string(ones);
    if (ones != 0)
        return std::to_string(tens) + "0" + std::to_string(ones);
    return std::to_string(tens) + "0";
}

}  // namespace pal4
