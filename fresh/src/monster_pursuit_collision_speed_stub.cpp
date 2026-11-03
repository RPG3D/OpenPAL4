#include "pal4/monster_pursuit_collision_speed.h"
#include "pal4/monster_behavior_manager.h"
namespace pal4 {
MonsterPursuitTarget MonsterPursuitSpeedRuntimeStub::Target(
    std::uint32_t) { return {}; }
std::uint32_t MonsterPursuitSpeedRuntimeStub::FindPursuitPath(
    float, std::array<float, 3>&) { return 0u; }
bool MonsterPursuitSpeedRuntimeStub::ApplyPursuitPath(
    std::uint32_t, const std::array<float, 3>&,
    const std::array<float, 3>&, std::uint32_t, std::uint32_t) {
    return false;
}
float MonsterPursuitSpeedRuntimeStub::MaxPursuitSpeed(
    std::uint32_t, float fallback) { return fallback; }
std::uint32_t MonsterPursuitSpeedRuntimeStub::SetPursuitSpeed(
    std::uint32_t, float) { return 0u; }
std::uint32_t MonsterPursuitSpeedRuntimeStub::ResetPursuit(
    MonsterBehaviorManagerState& state, std::uint32_t address) {
    MonsterPursuitRuntimeStub pursuit;
    return MonsterPursuitReset::Reset(state, address, pursuit);
}

} // namespace pal4
