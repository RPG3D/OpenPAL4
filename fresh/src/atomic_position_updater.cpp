#include "pal4/atomic_position_updater.h"

#include <bit>
#include <optional>

namespace pal4 {

namespace {

std::uint32_t UpdateSphere(
    const std::uint32_t atomic, const std::uint32_t destination,
    IAtomicPositionRuntime& runtime, const std::optional<bool> explicit_mode,
    std::uint32_t* initial_flag) {
    if ((runtime.ReadWord(atomic + 76u) & 2u) != 0u)
        runtime.ResyncInterpolatedSphere(atomic);

    AtomicSphere sphere{};
    for (std::uint32_t index = 0u; index < sphere.size(); ++index)
        sphere[index] = std::bit_cast<float>(
            runtime.ReadWord(atomic + 28u + index * 4u));

    AtomicMatrix transform{};
    const auto direct_transform = explicit_mode.has_value()
                                      ? *explicit_mode
                                      : runtime.UseDirectTransform();
    const auto object_transform = runtime.GetObjectTransform(
        runtime.ReadWord(atomic + 4u));
    if (direct_transform) {
        for (std::uint32_t index = 0u; index < transform.size(); ++index)
            transform[index] = runtime.ReadWord(object_transform + index * 4u);
    } else {
        const auto parent = runtime.ReadWord(atomic + 60u);
        const auto parent_transform = runtime.GetObjectTransform(
            runtime.ReadWord(parent + 4u));
        AtomicMatrix parent_matrix{};
        runtime.SetupTransformMatrix(parent_matrix, parent_transform);
        runtime.MultiplyMatrices(transform, object_transform, parent_matrix);
    }
    runtime.TransformSphere(sphere, transform);

    const auto is_initial = initial_flag != nullptr
                                ? *initial_flag != 0
                                : runtime.IsInitialSphere();
    if (is_initial) {
        for (std::uint32_t index = 0u; index < sphere.size(); ++index)
            runtime.WriteWord(destination + index * 4u,
                              std::bit_cast<std::uint32_t>(sphere[index]));
        if (initial_flag != nullptr) *initial_flag = 0;
        else runtime.ClearInitialSphere();
        return atomic;
    }

    AtomicSphere current{};
    for (std::uint32_t index = 0u; index < current.size(); ++index)
        current[index] = std::bit_cast<float>(
            runtime.ReadWord(destination + index * 4u));
    const AtomicVector3 delta{sphere[0u] - current[0u],
                              sphere[1u] - current[1u],
                              sphere[2u] - current[2u]};
    const auto distance = runtime.Length(delta);
    if (sphere[3u] + distance < current[3u]) return atomic;

    const auto radius_delta = sphere[3u] - current[3u];
    const auto inverse_distance = 1.0f / distance;
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto center = (sphere[index] + current[index] +
                             radius_delta * (delta[index] * inverse_distance)) *
                            0.5f;
        runtime.WriteWord(destination + index * 4u,
                          std::bit_cast<std::uint32_t>(center));
    }
    // Original667C60 adds current then incoming radius;5F9A40 reverses them.
    // Live game threads use x87 PC24, so preserve the per-operation float
    // rounding and this addition order rather than widening intermediates.
    const auto radius = explicit_mode.has_value()
        ? (distance + current[3u] + sphere[3u]) * 0.5f
        : (distance + sphere[3u] + current[3u]) * 0.5f;
    runtime.WriteWord(destination + 12u,
                      std::bit_cast<std::uint32_t>(radius));
    return atomic;
}

}  // namespace

std::uint32_t AtomicPositionUpdater::Update(
    const std::uint32_t atomic, const std::uint32_t destination,
    IAtomicPositionRuntime& runtime) {
    return UpdateSphere(atomic, destination, runtime, std::nullopt, nullptr);
}

std::uint32_t AtomicPositionUpdater::ProcessBoundingSphere(
    const std::uint32_t atomic, const std::uint32_t mode,
    std::uint32_t& initial, const std::uint32_t destination,
    IAtomicPositionRuntime& runtime) {
    return UpdateSphere(atomic, destination, runtime, mode != 0, &initial);
}

std::uint32_t AtomicPositionUpdater::UpdateCameraPosition(
    const std::uint32_t record, const std::uint32_t atomic,
    IAtomicPositionRuntime& runtime) {
    if (atomic == 0u) return 0u;
    const auto transform = runtime.GetObjectTransform(
        runtime.ReadWord(atomic + 4u));
    runtime.CalculatePosition(record + 64u, record + 48u, transform);
    const auto value = runtime.ReadWord(record + 60u);
    runtime.WriteWord(record + 76u, value);
    return value;
}

}  // namespace pal4
