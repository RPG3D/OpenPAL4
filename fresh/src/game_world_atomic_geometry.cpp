#include "pal4/game_world_atomic_geometry.h"

#include <bit>

namespace {

float AsFloat(const std::uint32_t bits) {
    return std::bit_cast<float>(bits);
}

std::uint32_t AsBits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

}  // namespace

namespace pal4 {

std::uint32_t GameWorldAtomicGeometry::Process(
    const std::uint32_t clump, const std::uint32_t transform_mode,
    GameWorldBoundsBits& bounds, GameWorldAtomicGeometryState& state,
    IGameWorldAtomicGeometryRuntime& runtime) {
    state.first_atomic = 1u;
    state.transform_mode = transform_mode;
    return runtime.ForAllAtomics(clump, bounds, state);
}

std::uint32_t GameWorldAtomicGeometry::UpdateAtomic(
    const std::uint32_t atomic, GameWorldBoundsBits& bounds,
    GameWorldAtomicGeometryState& state,
    IGameWorldAtomicGeometryRuntime& runtime) {
    const auto count = runtime.UserDataCount(atomic);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto entry = runtime.ReadUserData(atomic, index);
        if (entry.type == 3u && entry.text.size() > 5u &&
            entry.text.compare(0u, 5u, "hook_") == 0)
            return atomic;
    }

    if ((runtime.AtomicFlags(atomic) & 2u) != 0u)
        runtime.ResyncSphere(atomic);
    const auto sphere = runtime.ReadSphere(atomic);
    const auto frame = runtime.AtomicFrame(atomic);
    const auto first_matrix = runtime.ObjectTransform(frame);
    GameWorldMatrixBits matrix{};
    if (state.transform_mode != 0u) {
        matrix = first_matrix;
    } else {
        const auto parent_matrix = runtime.ObjectTransform(
            runtime.ParentFrame(atomic));
        const auto prepared = runtime.SetupTransformMatrix(parent_matrix);
        matrix = runtime.MultiplyMatrix(first_matrix, prepared);
    }
    const auto transformed = runtime.TransformSphere(sphere, matrix);
    if (state.first_atomic != 0u) {
        bounds = transformed;
        state.first_atomic = 0u;
        return atomic;
    }

    std::array<float, 3> delta{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        delta[axis] = AsFloat(transformed[axis]) - AsFloat(bounds[axis]);
    const auto distance = runtime.Distance(delta);
    const auto next_radius = AsFloat(transformed[3]);
    const auto old_radius = AsFloat(bounds[3]);
    if (next_radius + distance < old_radius)
        return atomic;

    const auto radius_delta = static_cast<double>(next_radius) - old_radius;
    for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
        const auto center_sum = static_cast<double>(AsFloat(transformed[axis])) +
                                AsFloat(bounds[axis]);
        const auto shifted = radius_delta * (delta[axis] / distance);
        bounds[axis] = AsBits(static_cast<float>((center_sum + shifted) * 0.5));
    }
    bounds[3] = AsBits(static_cast<float>(
        (distance + old_radius + next_radius) * 0.5));
    return atomic;
}

}  // namespace pal4
