#include "pal4/atomic_bounding_box_builder.h"

#include <bit>
#include <optional>

#include "pal4/atomic_position_updater.h"

namespace pal4 {

namespace {

std::uint32_t AddTransformedVerticesImpl(
    const std::uint32_t atomic, const std::uint32_t box,
    IAtomicBoundingBoxRuntime& runtime, const bool secondary_mode,
    const std::optional<bool> explicit_mode = std::nullopt) {
    const auto geometry = runtime.ReadWord(atomic + 24u);
    const auto count = static_cast<std::int32_t>(runtime.ReadWord(geometry + 20u));
    const auto morph = runtime.ReadWord(geometry + 92u);
    const auto source = runtime.ReadWord(morph + 20u);
    const auto points = runtime.Allocate(
        static_cast<std::uint32_t>(count) * 12u, 0u);

    AtomicMatrix transform{};
    bool direct_transform;
    if (explicit_mode.has_value()) {
        direct_transform = *explicit_mode;
    } else {
        direct_transform = secondary_mode
                               ? runtime.UseSecondaryDirectTransform()
                               : runtime.UseDirectTransform();
    }
    const auto object_transform =
        runtime.GetObjectTransform(runtime.ReadWord(atomic + 4u));
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

    runtime.TransformPoints(points, source, count, transform);
    for (std::int32_t index = 0; index < count; ++index)
        runtime.AddPoint(box, points + static_cast<std::uint32_t>(index) * 12u);
    runtime.Free(points);
    return atomic;
}

}  // namespace

std::uint32_t AtomicBoundingBoxBuilder::AddTransformedVertices(
    const std::uint32_t atomic, const std::uint32_t box,
    IAtomicBoundingBoxRuntime& runtime) {
    return AddTransformedVerticesImpl(atomic, box, runtime, false);
}

std::uint32_t AtomicBoundingBoxBuilder::AddTransformedVerticesSecondary(
    const std::uint32_t atomic, const std::uint32_t box,
    IAtomicBoundingBoxRuntime& runtime) {
    return AddTransformedVerticesImpl(atomic, box, runtime, true);
}

std::uint32_t AtomicBoundingBoxBuilder::ProcessBoundingBox(
    const std::uint32_t atomic, const std::uint32_t mode,
    const std::uint32_t box, IAtomicBoundingBoxRuntime& runtime) {
    return AddTransformedVerticesImpl(atomic, box, runtime, false, mode != 0);
}

std::uint32_t AtomicBoundingBoxBuilder::UpdateTransformedBox(
    const std::uint32_t box, const std::uint32_t atomic,
    IAtomicBoxTransformRuntime& runtime) {
    if (atomic == 0u) return 0u;
    const auto transform = runtime.GetObjectTransform(
        runtime.ReadWord(atomic + 4u));
    AtomicBoxCorners corners{};
    for (std::uint32_t corner = 0u; corner < 8u; ++corner) {
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            const auto source = box + axis * 4u +
                                ((corner & (1u << axis)) != 0u ? 0u : 12u);
            corners[corner * 3u + axis] =
                std::bit_cast<float>(runtime.ReadWord(source));
        }
    }
    runtime.TransformCorners(corners, transform);
    return runtime.CalculateBox(box + 24u, corners);
}

std::uint32_t AtomicBoundingBoxBuilder::InitializeClumpBounds(
    const std::uint32_t record, const std::uint32_t clump,
    IAtomicBoundsInitRuntime& runtime) {
    if (clump == 0u) return 0u;
    constexpr std::uint32_t kNegativeLimit = 0xff7fffffu;
    constexpr std::uint32_t kPositiveLimit = 0x7f7fffffu;
    for (const auto offset : {8u, 4u, 0u})
        runtime.WriteWord(record + offset, kNegativeLimit);
    for (const auto offset : {20u, 16u, 12u})
        runtime.WriteWord(record + offset, kPositiveLimit);
    (void)runtime.ForEachAtomicVertexBounds(clump, record);
    return runtime.ForEachAtomicPosition(clump, record + 48u);
}

std::uint32_t AtomicBoundingBoxBuilder::UpdateBoxAndCamera(
    const std::uint32_t record, const std::uint32_t atomic,
    IAtomicBoxTransformRuntime& box_runtime,
    IAtomicPositionRuntime& position_runtime) {
    (void)UpdateTransformedBox(record, atomic, box_runtime);
    return AtomicPositionUpdater::UpdateCameraPosition(
        record, atomic, position_runtime);
}

}  // namespace pal4
