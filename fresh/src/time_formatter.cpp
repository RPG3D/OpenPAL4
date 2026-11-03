#include "pal4/time_formatter.h"

namespace pal4 {

std::string TimeFormatter::FormatSeconds(const std::int32_t seconds) {
    const auto hours = seconds / 3600;
    const auto within_hour = seconds % 3600;
    const auto minutes = within_hour / 60;
    const auto remaining_seconds = within_hour % 60;

    auto result = std::to_string(hours);
    result += minutes <= 9 ? ": 0" : ": ";
    result += std::to_string(minutes);
    result += remaining_seconds <= 9 ? ": 0" : ": ";
    result += std::to_string(remaining_seconds);
    return result;
}

}  // namespace pal4
