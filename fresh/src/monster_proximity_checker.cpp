#include "pal4/monster_proximity_checker.h"

#include "pal4/monster_behavior_manager.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

float Position(const MonsterBehaviorManagerState& state,
               std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

}  // namespace

MonsterProximityActor MonsterProximityRuntimeStub::ControlledActor() {
    return {};
}

double MonsterProximityRuntimeStub::PlanarDistance(float dx, float dz) {
    const float squared = dx * dx + dz * dz;
    return std::sqrt(static_cast<double>(squared));
}

float MonsterProximityRuntimeStub::DetectionRadius(std::uint32_t) {
    return 0.0f;
}

std::uint32_t MonsterProximityRuntimeStub::CollisionClump() {
    return 0u;
}

bool MonsterProximityRuntimeStub::CollisionBlocked(
    const std::array<float, 6>&, std::uint32_t) {
    return false;
}

void MonsterProximityRuntimeStub::NotifyReturnTargetReached(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

bool MonsterProximityChecker::Check(MonsterBehaviorManagerState& state,
                                    IMonsterProximityRuntime& runtime) {
    state.words[0x6e4u / 4u] = 0u;
    const auto actor = runtime.ControlledActor();
    if (actor.address == 0u || !actor.active)
        return false;

    const auto dx = actor.position[0] - Position(state, 0xacu);
    const auto dz = actor.position[2] - Position(state, 0xb4u);
    const auto distance = runtime.PlanarDistance(dx, dz);
    const auto radius = runtime.DetectionRadius(state.words[0x654u / 4u]);
    if (!(distance <= static_cast<double>(radius)))
        return false;

    const auto height_difference = std::fabs(
        static_cast<double>(actor.position[1]) - Position(state, 0xb0u));
    if (!(static_cast<double>(actor.half_height) * 2.0 > height_difference))
        return false;

    const std::array<float, 6> endpoints{
        Position(state, 0xe0u), Position(state, 0xe4u),
        Position(state, 0xe8u), actor.collision_position[0],
        actor.collision_position[1], actor.collision_position[2]};
    if (runtime.CollisionBlocked(endpoints, runtime.CollisionClump()))
        return false;

    state.words[0x6e4u / 4u] = actor.address;
    for (std::uint32_t axis = 0u; axis != 3u; ++axis)
        state.words[(0x6e8u + axis * 4u) / 4u] =
            state.words[(0xacu + axis * 4u) / 4u];
    return true;
}

bool MonsterProximityChecker::BeyondReturnThreshold(
    const MonsterBehaviorManagerState& state,
    IMonsterProximityRuntime& runtime) {
    const auto dx = Position(state, 0x6e8u) - Position(state, 0xacu);
    const auto dz = Position(state, 0x6f0u) - Position(state, 0xb4u);
    return !(runtime.PlanarDistance(dx, dz) < 1000.0);
}

bool MonsterProximityChecker::CheckReturnTargetReached(
    const MonsterBehaviorManagerState& state,
    std::uint32_t manager_address, IMonsterProximityRuntime& runtime) {
    const auto dx = Position(state, 0x6e8u) - Position(state, 0xacu);
    const auto dz = Position(state, 0x6f0u) - Position(state, 0xb4u);
    if (!(runtime.PlanarDistance(dx, dz) <=
          static_cast<double>(0.05f)))
        return false;
    runtime.NotifyReturnTargetReached(
        manager_address, manager_address + 0x6e8u, 0u);
    return true;
}

}  // namespace pal4
