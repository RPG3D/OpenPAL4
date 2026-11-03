#include "pal4/object_path_movement.h"

#include <cmath>

namespace pal4 {
namespace {

void SetActive(ObjectMovementState& state, bool active) {
    state.words[46] = (state.words[46] & 0xffff00ffu) |
                      (active ? 0x100u : 0u);
}

}  // namespace

BehaviorVector3 ObjectPathMovementRuntimeStub::Position(
    ObjectMovementState&) { return {}; }
float ObjectPathMovementRuntimeStub::DistanceAlong(
    std::uint32_t, const BehaviorVector3&) { return 0.0f; }
float ObjectPathMovementRuntimeStub::Speed(ObjectMovementState&) { return 0.0f; }
float ObjectPathMovementRuntimeStub::PathLength(std::uint32_t) { return 0.0f; }
BehaviorVector3 ObjectPathMovementRuntimeStub::PointAt(
    std::uint32_t, float) { return {}; }
BehaviorVector3 ObjectPathMovementRuntimeStub::LastPoint(
    std::uint32_t) { return {}; }
void ObjectPathMovementRuntimeStub::SetPosition(
    ObjectMovementState&, const BehaviorVector3&) {}
std::uint32_t ObjectPathMovementRuntimeStub::NotifyMember(
    std::uint32_t, const BehaviorVector3&) { return 0u; }

std::uint32_t ObjectPathMovement::Advance(
    ObjectMovementState& state, std::uint32_t /*unused*/, float delta_time,
    IObjectPathMovementRuntime& runtime) {
    if (state.words[47] == 0u) {
        SetActive(state, false);
        return 0u;
    }

    const auto path_address = state.words[47];
    const auto path_distance = runtime.DistanceAlong(
        path_address, runtime.Position(state));
    const auto target_distance =
        path_distance + runtime.Speed(state) * delta_time;
    const auto length = runtime.PathLength(state.words[47]);
    const auto path_point = !(length <= target_distance)
        ? runtime.PointAt(state.words[47], target_distance)
        : runtime.LastPoint(state.words[47]);

    const auto current = runtime.Position(state);
    BehaviorVector3 direction{
        path_point[0] - current[0],
        path_point[1] - current[1],
        path_point[2] - current[2]};
    const auto magnitude = std::sqrt(direction[0] * direction[0] +
                                     direction[1] * direction[1] +
                                     direction[2] * direction[2]);
    if (magnitude > 0.0f) {
        for (auto& component : direction) component /= magnitude;
    }

    const auto step = runtime.Speed(state) * delta_time;
    const auto latest = runtime.Position(state);
    BehaviorVector3 next{
        latest[0] + direction[0] * step,
        latest[1] + direction[1] * step,
        latest[2] + direction[2] * step};
    runtime.SetPosition(state, next);
    const auto result = runtime.NotifyMember(state.words[42], next);
    SetActive(state, true);
    return result;
}

}  // namespace pal4
