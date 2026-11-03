#pragma once

#include <cstdint>

namespace pal4::portable {

// Uses the same health condition rule as native CombatActorStatus.
[[nodiscard]] std::int32_t BattleHealthState(
    std::int32_t current_health, std::int32_t maximum_health) noexcept;

}  // namespace pal4::portable
