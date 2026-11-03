#include "pal4/monster_object_interaction_callback.h"

#include <cmath>

namespace pal4 {

MonsterInteractionContext MonsterObjectInteractionRuntimeStub::Context(
    std::uint32_t) { return {}; }
MonsterInteractionObject MonsterObjectInteractionRuntimeStub::Candidate(
    std::uint32_t) { return {}; }
double MonsterObjectInteractionRuntimeStub::TargetDoubleProperty(
    std::uint32_t) { return 0.0; }
double MonsterObjectInteractionRuntimeStub::PlanarDistance(float dx, float dz) {
    const float squared = dx * dx + dz * dz;
    return std::sqrt(static_cast<double>(squared));
}
float MonsterObjectInteractionRuntimeStub::InteractionRadius(
    std::uint32_t) { return 0.0f; }
std::uint32_t MonsterObjectInteractionRuntimeStub::CollisionClump() {
    return 0u;
}
bool MonsterObjectInteractionRuntimeStub::CollisionBlocked(
    const std::array<float, 6>&, std::uint32_t) { return false; }
void MonsterObjectInteractionRuntimeStub::EnterPursuit(
    std::uint32_t, std::uint32_t) {}

std::uint32_t MonsterObjectInteractionCallback::Process(
    std::uint32_t candidate_address, std::uint32_t manager_address,
    IMonsterObjectInteractionRuntime& runtime) {
    if (candidate_address == manager_address)
        return candidate_address;

    const auto context = runtime.Context(manager_address);
    const auto candidate = runtime.Candidate(candidate_address);
    const auto vertical_difference = std::fabs(
        static_cast<double>(context.position[1]) - candidate.position[1]);
    const auto vertical_reach = 2.0 *
        runtime.TargetDoubleProperty(context.target_address);
    const bool vertically_reachable = vertical_reach > vertical_difference;

    const auto dx = candidate.position[0] - context.position[0];
    const auto dz = candidate.position[2] - context.position[2];
    const auto distance = runtime.PlanarDistance(dx, dz);
    const auto radius = runtime.InteractionRadius(context.source_address);
    if (!(distance <= static_cast<double>(radius)) ||
        !vertically_reachable)
        return candidate_address;

    const std::array<float, 6> endpoints{
        context.collision_position[0], context.collision_position[1],
        context.collision_position[2], candidate.collision_position[0],
        candidate.collision_position[1], candidate.collision_position[2]};
    if (!runtime.CollisionBlocked(endpoints, runtime.CollisionClump()))
        runtime.EnterPursuit(candidate_address, context.target_address);
    return candidate_address;
}

}  // namespace pal4
