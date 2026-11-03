#include "pal4/move_to_target_task.h"

#include <bit>

namespace pal4 {
namespace {

float ReadFloat(const MoveToTargetTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

void WriteFloat(MoveToTargetTaskState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}

}  // namespace

void MoveToTargetTask::InitializeTrajectory(
    MoveToTargetTaskState& state,
    IMoveToTargetTrajectoryRuntime& runtime) {
    const auto object = state.words[16u];
    const auto facet = runtime.ResolveMoveFacet(object, ClassName());
    state.words[31u] = facet;
    const auto origin = runtime.ReadFacetPosition(facet);
    const auto offset = runtime.ReadTargetOffset(state.words[17u], state);
    std::array<float, 3u> target{
        ReadFloat(state, 21u) + offset[0u],
        ReadFloat(state, 22u) + offset[1u],
        ReadFloat(state, 23u) + offset[2u]};

    const auto transform = runtime.GetObjectTransform(object);
    std::array<std::uint32_t, 16u> matrix{};
    runtime.SetupTransformMatrix(matrix, transform);
    runtime.MultiplyOwnerMatrix(matrix, object);
    runtime.TransformPosition(target, matrix);

    for (std::size_t axis = 0u; axis < 3u; ++axis)
        WriteFloat(state, 21u + axis, target[axis]);

    std::array<float, 3u> displacement{};
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        displacement[axis] = target[axis] - origin[axis];
    if ((state.words[34u] & 0xffu) != 0u)
        WriteFloat(state, 5u,
                   runtime.VectorLength(displacement) / ReadFloat(state, 35u));

    const auto inverse_duration = 1.0f / ReadFloat(state, 5u);
    for (std::size_t axis = 0u; axis < 3u; ++axis)
        WriteFloat(state, 18u + axis,
                   displacement[axis] * inverse_duration);
}

}  // namespace pal4
