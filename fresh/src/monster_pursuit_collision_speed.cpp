#include "pal4/monster_pursuit_collision_speed.h"

#include "pal4/monster_behavior_manager.h"

#include <bit>

namespace pal4 {
namespace {

std::array<float, 3> ManagerPosition(
    const MonsterBehaviorManagerState& state) {
    return {
        std::bit_cast<float>(state.words[0xacu / 4u]),
        std::bit_cast<float>(state.words[0xb0u / 4u]),
        std::bit_cast<float>(state.words[0xb4u / 4u])};
}

}  // namespace

bool MonsterPursuitCollisionSpeed::Process(
    const MonsterBehaviorManagerState& state,
    std::uint32_t manager_address, float elapsed,
    std::uint32_t target_address, IMonsterPursuitSpeedRuntime& runtime) {
    if (target_address == 0u)
        return false;
    auto target = runtime.Target(target_address);
    if (!target.active)
        return false;

    const auto manager_position = ManagerPosition(state);
    const auto path = runtime.FindPursuitPath(elapsed, target.position);
    if (path == 0u ||
        !runtime.ApplyPursuitPath(manager_address + 0x564u,
                                  manager_position, target.position,
                                  path, 0u))
        return false;

    constexpr float default_speed = 50.0f;
    const auto speed = runtime.MaxPursuitSpeed(
        state.words[0x654u / 4u], default_speed);
    (void)runtime.SetPursuitSpeed(manager_address + 0x564u, speed);
    return true;
}

std::uint32_t MonsterPursuitReturnState::Enter(
    MonsterBehaviorManagerState& state, std::uint32_t manager_address,
    float elapsed, IMonsterPursuitReturnRuntime& runtime) {
    state.words[0x6fcu / 4u] = state.words[0x6f8u / 4u];
    state.words[0x6e0u / 4u] = 2u;
    state.words[0x6e4u / 4u] = 0u;

    const auto manager_position = ManagerPosition(state);
    auto saved_position = std::array<float, 3>{
        std::bit_cast<float>(state.words[0x6e8u / 4u]),
        std::bit_cast<float>(state.words[0x6ecu / 4u]),
        std::bit_cast<float>(state.words[0x6f0u / 4u])};
    const auto path = runtime.FindPursuitPath(elapsed, saved_position);
    std::uint32_t result = path;
    if (path != 0u) {
        if (runtime.ApplyPursuitPath(manager_address + 0x564u,
                                     manager_position, saved_position,
                                     path, 0u)) {
            const auto speed = runtime.MaxPursuitSpeed(
                state.words[0x654u / 4u], 50.0f);
            result = runtime.SetPursuitSpeed(
                manager_address + 0x564u, speed);
        } else {
            result = runtime.ResetPursuit(state, manager_address);
        }
    }
    state.words[0x6e4u / 4u] = 0u;
    return result;
}

}  // namespace pal4
