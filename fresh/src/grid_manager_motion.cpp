#include "pal4/grid_manager.h"

#include <bit>
#include <cmath>
#include <cstdint>

namespace pal4 {
namespace {

GridVector3 ReadTriple(const GridManagerState& state,
                       const std::uint32_t first_word) {
    return {std::bit_cast<float>(state.base_words[first_word]),
            std::bit_cast<float>(state.base_words[first_word + 1u]),
            std::bit_cast<float>(state.base_words[first_word + 2u])};
}

void WriteTriple(GridManagerState& state, const std::uint32_t first_word,
                 const GridVector3& value) {
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        state.base_words[first_word + axis] =
            std::bit_cast<std::uint32_t>(value[axis]);
}

double LengthSquared(const GridVector3& value) {
    return static_cast<double>(value[0]) * value[0] +
           static_cast<double>(value[1]) * value[1] +
           static_cast<double>(value[2]) * value[2];
}

GridVector3 LimitLength(const GridVector3& value, const float limit) {
    const double squared = LengthSquared(value);
    if (squared <= static_cast<double>(limit) * limit)
        return value;
    const double scale = static_cast<double>(limit) / std::sqrt(squared);
    return {static_cast<float>(value[0] * scale),
            static_cast<float>(value[1] * scale),
            static_cast<float>(value[2] * scale)};
}

}  // namespace

float GridManager::ClampValue(const float value, const float lower,
                              const float upper) {
    if (value < lower)
        return lower;
    if (value >= upper)
        return upper;
    return value;
}

GridVector3 GridManager::InterpolateVector(
    const float factor, const GridVector3& first,
    const GridVector3& second) {
    return {static_cast<float>(first[0] +
                               (second[0] - first[0]) * factor),
            static_cast<float>(first[1] +
                               (second[1] - first[1]) * factor),
            static_cast<float>(first[2] +
                               (second[2] - first[2]) * factor)};
}

GridManagerState& GridManager::UpdatePositionAndVelocity(
    GridManagerState& state, const float input, const float delta_time,
    IGridManagerMotionRuntime& runtime) {
    const auto requested_acceleration =
        runtime.CalculateAcceleration(state, input, delta_time);
    const auto acceleration_limit = runtime.ReadScalar(state, 112u);
    const auto acceleration = LimitLength(requested_acceleration,
                                           acceleration_limit);
    const float mass = runtime.ReadScalar(state, 80u);
    GridVector3 scaled_acceleration{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        scaled_acceleration[axis] = acceleration[axis] / mass;
    const auto previous_velocity = runtime.ReadVelocity(state);
    if (delta_time > 0.0f) {
        const float blend = ClampValue(
            ClampValue(delta_time * 9.0f, 0.15000001f, 0.40000001f),
            0.0f, 1.0f);
        WriteTriple(state, 39u, InterpolateVector(
            blend, ReadTriple(state, 39u), scaled_acceleration));
    }
    const auto current_acceleration = ReadTriple(state, 39u);
    GridVector3 velocity{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        velocity[axis] = previous_velocity[axis] +
                         delta_time * current_acceleration[axis];
    velocity = LimitLength(velocity, runtime.ReadScalar(state, 120u));
    runtime.SetSpeed(state, static_cast<float>(
        std::sqrt(LengthSquared(velocity))));
    const auto old_position = runtime.ReadPosition(state);
    GridVector3 next_position{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        next_position[axis] = old_position[axis] +
                              delta_time * velocity[axis];
    runtime.SetPosition(state, next_position);
    runtime.CommitVelocity(state, velocity, delta_time);
    (void)UpdatePhysics(state, delta_time, runtime);
    const auto final_position = runtime.ReadPosition(state);
    WriteTriple(state, 35u, InterpolateVector(
        ClampValue(delta_time * 0.059999999f, 0.0f, 1.0f),
        ReadTriple(state, 35u), final_position));
    return state;
}

std::uint32_t GridManager::SetVelocityDirection(
    GridManagerState& state, const GridVector3& velocity,
    IGridManagerVelocityRuntime& runtime) {
    const float speed = runtime.ReadSpeed(state);
    if (!(speed > 0.0f))
        return 0u;
    const float current_speed = runtime.ReadSpeed(state);
    GridVector3 direction{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        direction[axis] = velocity[axis] / current_speed;
    return runtime.ApplyDirection(state, direction);
}

GridVector3 GridManager::CalculatePosition(
    GridManagerState& state, const float delta_time,
    IGridManagerMotionRuntime& runtime) {
    const auto velocity = runtime.ReadVelocity(state);
    const auto position = runtime.ReadPosition(state);
    return {position[0] + delta_time * velocity[0],
            position[1] + delta_time * velocity[1],
            position[2] + delta_time * velocity[2]};
}

}  // namespace pal4
