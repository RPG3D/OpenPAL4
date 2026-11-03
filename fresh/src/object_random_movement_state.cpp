#include "pal4/object_random_movement_state.h"

#include "pal4/vector3_operations.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

float FloatAt(const ObjectRandomMovementState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

void WriteDirection(ObjectRandomMovementState& state,
                    const BehaviorVector3& direction) {
    for (std::size_t index = 0; index < 3u; ++index)
        state.words[50u + index] = std::bit_cast<std::uint32_t>(direction[index]);
}

}  // namespace

BehaviorVector3 ObjectRandomMovementSpatialRuntimeStub::Position(
    ObjectRandomMovementState&) { return {}; }
BehaviorVector3 ObjectRandomMovementSpatialRuntimeStub::RandomVector() {
    return {};
}

bool ObjectRandomMovementProximity::IsOutsideTargetRadius(
    ObjectRandomMovementState& state,
    IObjectRandomMovementSpatialRuntime& runtime) {
    const auto position = runtime.Position(state);
    const double dx = static_cast<double>(position[0]) - FloatAt(state, 53u);
    const double dy = static_cast<double>(position[1]) - FloatAt(state, 54u);
    const double dz = static_cast<double>(position[2]) - FloatAt(state, 55u);
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    return distance >= FloatAt(state, 47u);
}

std::uint32_t ObjectRandomMovementStateUpdate::Update(
    ObjectRandomMovementState& state, float delta_time,
    IObjectRandomMovementSpatialRuntime& runtime) {
    if (ObjectRandomMovementProximity::IsOutsideTargetRadius(state, runtime)) {
        const auto position = runtime.Position(state);
        BehaviorVector3 direction{
            FloatAt(state, 53u) - position[0],
            FloatAt(state, 54u) - position[1],
            FloatAt(state, 55u) - position[2]};
        const auto original_z = std::bit_cast<std::uint32_t>(direction[2]);
        WriteDirection(state, direction);
        const double length = std::sqrt(
            static_cast<double>(direction[0]) * direction[0] +
            static_cast<double>(direction[1]) * direction[1] +
            static_cast<double>(direction[2]) * direction[2]);
        if (!(length > 0.0)) return original_z;
        BehaviorVector3 normalized{};
        Vector3Operations::Normalize(direction, normalized);
        WriteDirection(state, normalized);
        return 0u;
    }

    const auto elapsed = FloatAt(state, 49u);
    if (!(elapsed >= FloatAt(state, 48u))) {
        state.words[49] = std::bit_cast<std::uint32_t>(elapsed + delta_time);
        return 0x100u;
    }

    const auto random = runtime.RandomVector();
    const BehaviorVector3 horizontal{random[0], 0.0f, random[2]};
    BehaviorVector3 direction{};
    Vector3Operations::Normalize(horizontal, direction);
    WriteDirection(state, direction);
    state.words[49] = 0u;
    return std::bit_cast<std::uint32_t>(direction[1]);
}

}  // namespace pal4
