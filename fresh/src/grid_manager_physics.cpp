#include "pal4/grid_manager.h"
#include "movement_vector_math.h"

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

}  // namespace

std::uint32_t GridManager::UpdatePhysics(
    GridManagerState& state, const float delta_time,
    IGridManagerMotionRuntime& runtime) {
    if (!(delta_time > 0.0f))
        return 0u;
    const auto current_position = runtime.ReadPosition(state);
    const auto previous_position = ReadTriple(state, 32u);
    GridVector3 displacement{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        displacement[axis] = previous_position[axis] -
                             current_position[axis];
    const auto previous_normal = ReadTriple(state, 29u);
    const auto current_normal = runtime.ReadNormal(state);
    GridVector3 normal_delta{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        normal_delta[axis] = previous_normal[axis] -
                             current_normal[axis];
    const float distance=movement_detail::Length(displacement);
    const volatile float inverse_distance=1.f/distance;
    GridVector3 direction{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        direction[axis] = normal_delta[axis]*inverse_distance;
    const auto projection_normal = runtime.ReadNormal(state);
    const float projection=movement_detail::Dot(direction,projection_normal);
    GridVector3 tangent{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
    {
        const volatile float projected=projection*projection_normal[axis];
        tangent[axis]=direction[axis]-projected;
    }
    const auto orientation = runtime.ReadOrientation(state);
    // 65F5A0 associates Y,Z,X in both the sign dot and tangent norm.
    const GridVector3 ordered_tangent{tangent[0],tangent[2],tangent[1]};
    const GridVector3 ordered_orientation{orientation[0],orientation[2],orientation[1]};
    const float sign=movement_detail::Dot(ordered_tangent,ordered_orientation)>=0.f?-1.f:1.f;
    const float signed_length=sign*movement_detail::Length(ordered_tangent);
    state.base_words[28] = std::bit_cast<std::uint32_t>(signed_length);
    const float old_smoothed = std::bit_cast<float>(state.base_words[38]);
    const float factor = ClampValue(delta_time * 4.0f, 0.0f, 1.0f);
    state.base_words[38] = std::bit_cast<std::uint32_t>(
        old_smoothed + (signed_length - old_smoothed) * factor);
    WriteTriple(state, 29u, runtime.ReadNormal(state));
    const auto final_position = runtime.ReadPosition(state);
    WriteTriple(state, 32u, final_position);
    return state.base_words[34];
}

}  // namespace pal4
