#include "pal4/effect_system_radius.h"

#include <cmath>

namespace pal4 {

std::uint8_t EffectSystemRadius::SetTimeAndFlag(
    std::uint32_t auxiliary, float radius,
    IEffectSystemRadiusRuntime& runtime) {
    runtime.CleanupEffects(auxiliary);
    runtime.WriteByte(auxiliary + 4u, 1u);
    runtime.WriteFloat(auxiliary + 8u, radius * radius);
    return 1u;
}

std::optional<std::uint32_t> EffectSystemRadius::SetSquaredValue(
    std::uint32_t auxiliary, float radius,
    IEffectSystemRadiusRuntime& runtime) {
    if (runtime.ReadByte(auxiliary + 4u) == 0u)
        return std::nullopt;
    if (std::isnan(radius))
        return 0x4100u;
    if (radius == 0.0f)
        return 0x4000u;
    if (radius < 0.0f)
        return 0x100u;
    runtime.WriteFloat(auxiliary + 8u, radius * radius);
    return 0u;
}

}  // namespace pal4
