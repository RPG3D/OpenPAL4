#include "pal4/monster_pursuit_collision_gate.h"

#include "pal4/monster_behavior_manager.h"

namespace pal4 {

bool MonsterPursuitCollisionGateRuntimeStub::ProcessPursuit(
    const MonsterBehaviorManagerState& state, std::uint32_t address,
    float elapsed, std::uint32_t target) {
    return MonsterPursuitCollisionSpeed::Process(
        state, address, elapsed, target, pursuit_);
}

std::uint32_t MonsterPursuitCollisionGateRuntimeStub::EnterReturn(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    float elapsed) {
    return MonsterPursuitReturnState::Enter(
        state, address, elapsed, pursuit_);
}

std::uint32_t MonsterPursuitCollisionGateRuntimeStub::PlayerControlObject() {
    return 0u;
}

bool MonsterPursuitCollisionGateRuntimeStub::PlayerControlFlag(
    std::uint32_t) { return false; }

}  // namespace pal4
