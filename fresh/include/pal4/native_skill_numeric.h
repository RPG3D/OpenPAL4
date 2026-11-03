#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
namespace pal4 {
// Public host conversion, retaining the original x87 invalid 64-bit result.
inline std::int64_t TruncateSkillFloat(double number) {
    if(!std::isfinite(number) || number < -9223372036854775808.0 || number >= 9223372036854775808.0)
        return std::numeric_limits<std::int64_t>::min();
    return static_cast<std::int64_t>(number);
}
inline std::int64_t TruncateSkillFloat(float value) {
    return TruncateSkillFloat(static_cast<double>(value));
}
}
