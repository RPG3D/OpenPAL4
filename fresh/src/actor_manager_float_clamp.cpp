#include "pal4/actor_manager_float_clamp.h"

#include <cmath>

namespace pal4 {

std::uint32_t ActorManagerFloatClamp::Clamp(
    float& value, const float lower, const float upper) {
    if (value < lower) value = lower;
    if (std::isnan(value) || std::isnan(upper)) return 0x4100u;
    if (value < upper) return 0x0100u;
    if (value == upper) return 0x4000u;
    value = upper;
    return 0u;
}

}  // namespace pal4
