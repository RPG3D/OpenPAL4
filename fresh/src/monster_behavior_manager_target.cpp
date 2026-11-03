#include "pal4/monster_behavior_manager.h"

namespace pal4 {

void MonsterBehaviorManager::ClearStopPursuitFlag(
    MonsterBehaviorManagerState& state) {
    state.words[0x704u / 4u] = 0u;
}

std::uint32_t MonsterBehaviorManager::SetPursuitTargetIfActive(
    MonsterBehaviorManagerState& state, const std::uint32_t target) {
    if (state.words[0x1f0u / 4u] == 0u) return 0u;
    if (target != 0u && state.words[0x6e0u / 4u] == 1u)
        state.words[0x6e4u / 4u] = target;
    return target;
}

std::uint32_t MonsterBehaviorManager::SetStopPursuitFlag(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    float incoming_st0, IMonsterStopPursuitRuntime& runtime) {
    state.words[0x704u / 4u] = 1u;
    if (state.words[0x730u / 4u] != 1u && state.words[0x6e0u / 4u] == 1u)
        return runtime.ProcessCollisionAndConfig(address, incoming_st0, 1.0f);
    return 1u;
}

}  // namespace pal4
