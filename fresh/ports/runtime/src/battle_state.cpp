#include "pal4/portable/battle_state.h"

#include "pal4/combat_health_state.h"

namespace pal4::portable {

std::int32_t BattleHealthState(std::int32_t current_health,
                               std::int32_t maximum_health) noexcept {
    return pal4::CombatHealthState(current_health, maximum_health);
}

}  // namespace pal4::portable
