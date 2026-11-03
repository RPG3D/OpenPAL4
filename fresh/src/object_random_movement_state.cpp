#include "pal4/object_random_movement_state.h"

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

float LengthInOrder(float first, float second, float third) {
    // Original game x87 PC24 rounds each multiply/add and FSQRT. Callers
    // preserve their different operand associations; no RenderWare call here.
    const volatile float first_squared = first * first;
    const volatile float second_squared = second * second;
    const volatile float pair = first_squared + second_squared;
    const volatile float third_squared = third * third;
    const volatile float sum = pair + third_squared;
    return static_cast<float>(std::sqrt(static_cast<double>(sum)));
}

bool NormalizeDirection(BehaviorVector3& direction) {
    // CalculateDotProduct40BF20: (Z*Z + Y*Y) + X*X, followed by
    // 42CEA9/42CF52 FSQRT and reciprocal at PC24.
    const auto length = LengthInOrder(direction[2], direction[1], direction[0]);
    if (!(length > 0.f)) return false;
    const volatile float inverse = 1.f / length;
    for (auto& component : direction) component = inverse * component;
    return true;
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
    const float dx = position[0] - FloatAt(state, 53u);
    const float dy = position[1] - FloatAt(state, 54u);
    const float dz = position[2] - FloatAt(state, 55u);
    // 42D002..42D032 differs from CalculateDotProduct: Z,X,Y.
    const auto distance = LengthInOrder(dz, dx, dy);
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
        if (!NormalizeDirection(direction)) return original_z;
        WriteDirection(state, direction);
        return 0u;
    }

    const auto elapsed = FloatAt(state, 49u);
    if (!(elapsed >= FloatAt(state, 48u))) {
        state.words[49] = std::bit_cast<std::uint32_t>(elapsed + delta_time);
        return 0x100u;
    }

    const auto random = runtime.RandomVector();
    BehaviorVector3 direction{random[0], 0.0f, random[2]};
    (void)NormalizeDirection(direction);
    WriteDirection(state, direction);
    state.words[49] = 0u;
    return std::bit_cast<std::uint32_t>(direction[1]);
}

}  // namespace pal4
