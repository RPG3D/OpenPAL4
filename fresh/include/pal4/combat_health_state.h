#pragma once

#include <cstdint>

namespace pal4 {

// Mirrors the native actor health condition states: 0 healthy, 1 wounded,
// 2 critical, and 3 no positive health.
[[nodiscard]] inline std::int32_t CombatHealthState(
    std::int32_t current_health, std::int32_t maximum_health) noexcept {
    if (current_health <= 0) return 3;

    const auto ratio = static_cast<double>(static_cast<float>(current_health)) /
                       static_cast<double>(maximum_health);
    if (ratio <= static_cast<double>(0.2f)) return 2;
    if (ratio <= static_cast<double>(0.4f)) return 1;
    return 0;
}

}  // namespace pal4
