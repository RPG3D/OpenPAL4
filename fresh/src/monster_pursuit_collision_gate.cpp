#include "pal4/monster_pursuit_collision_gate.h"

#include "pal4/monster_behavior_manager.h"

namespace pal4 {

std::uint32_t MonsterPursuitCollisionGate::Process(
    MonsterBehaviorManagerState& state, std::uint32_t manager_address,
    float elapsed, IMonsterPursuitCollisionGateRuntime& runtime) {
    const auto active = state.words[0x1f0u / 4u];
    if (active == 0u || state.words[0x730u / 4u] == 1u)
        return active;

    const auto pursuit_state = state.words[0x6e0u / 4u];
    if (pursuit_state == 2u)
        return runtime.EnterReturn(state, manager_address, elapsed);
    if (pursuit_state != 1u)
        return pursuit_state - 2u;

    const auto target = state.words[0x6e4u / 4u];
    if (target == 0u ||
        !runtime.ProcessPursuit(state, manager_address, elapsed, target))
        return runtime.EnterReturn(state, manager_address, elapsed);
    const auto player = runtime.PlayerControlObject();
    if (runtime.PlayerControlFlag(player))
        return runtime.EnterReturn(state, manager_address, elapsed);
    return player;
}

}  // namespace pal4
