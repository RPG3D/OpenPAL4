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

double Dot(const GridVector3& first, const GridVector3& second) {
    return static_cast<double>(first[0]) * second[0] +
           static_cast<double>(first[1]) * second[1] +
           static_cast<double>(first[2]) * second[2];
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
    const double distance = std::sqrt(Dot(displacement, displacement));
    GridVector3 direction{};
    if (distance > 0.0) {
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            direction[axis] = static_cast<float>(
                normal_delta[axis] / distance);
    }
    const auto projection_normal = runtime.ReadNormal(state);
    const double projection = Dot(direction, projection_normal);
    GridVector3 tangent{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        tangent[axis] = static_cast<float>(
            direction[axis] - projection * projection_normal[axis]);
    const auto orientation = runtime.ReadOrientation(state);
    const double sign = Dot(tangent, orientation) >= 0.0 ? -1.0 : 1.0;
    const float signed_length = static_cast<float>(
        sign * std::sqrt(Dot(tangent, tangent)));
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
