#include "pal4/combat_monster_creation.h"
#include <array>
#include <vector>
namespace pal4 {
void CombatMonsterCreation::Create(std::uint32_t config,std::int32_t child_count,ICombatMonsterCreationRuntime& runtime){
    std::array<std::uint32_t, 6> occupied{};
    std::vector<std::uint32_t> deferred;
    for (std::int32_t index = 0; index < child_count; ++index) {
        const auto child = runtime.GetChildByIndex(
            config, static_cast<std::uint32_t>(index));
        if (child == 0u) continue;
        const auto monster_id = runtime.GetConfigInt(child, "MonsterId", -1);
        const auto position = static_cast<std::uint32_t>(
            runtime.GetConfigInt(child, "CombatPosition", 6));
        if (monster_id == 0) continue;
        if (position > 5u || occupied[position] != 0u) {
            deferred.push_back(child);
            continue;
        }
        occupied[position] = child;
        runtime.CreateMonster(monster_id,position,runtime.GetConfigInt(child,"MonsterAbState",0));
    }
    for (const auto child : deferred) {
        const auto monster_id = runtime.GetConfigInt(child, "MonsterId", -1);
        runtime.CreateMonster(monster_id,6u,runtime.GetConfigInt(child,"MonsterAbState",0));
    }

}
}
