#include "pal4/thunder_visual_transform.h"

#include <bit>
#include <cstdint>

namespace pal4 {
namespace {

ThunderVector3 ReadVector(const ThunderObjectState& state,
                          std::uint32_t offset) {
    offset += 60u;
    return {std::bit_cast<float>(state.words[offset / 4u]),
            std::bit_cast<float>(state.words[offset / 4u + 1u]),
            std::bit_cast<float>(state.words[offset / 4u + 2u])};
}

void WriteVector(ThunderObjectState& state, std::uint32_t offset,
                 ThunderVector3 value) {
    offset += 60u;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[offset / 4u + i] = std::bit_cast<std::uint32_t>(value[i]);
}

ThunderVector3 Subtract(ThunderVector3 left, ThunderVector3 right) {
    return {left[0] - right[0], left[1] - right[1],
            left[2] - right[2]};
}

ThunderVector3 Cross(ThunderVector3 left, ThunderVector3 right) {
    return {left[1] * right[2] - left[2] * right[1],
            left[2] * right[0] - left[0] * right[2],
            left[0] * right[1] - left[1] * right[0]};
}

}  // namespace

std::uint32_t ThunderVisualTransform::Update(
    ThunderObjectState& state, IThunderVisualTransformRuntime& runtime) {
    const auto source_frame = state.words[28u];
    const auto source_transform = runtime.FrameTransform(source_frame);
    const auto origin = runtime.MatrixPosition(source_transform);
    const auto local_target = ReadVector(state, 88u);

    ThunderVector3 target{};
    const auto target_transform = runtime.FrameTransform(source_frame);
    if (((state.words[15u] >> 16u) & 0xffu) != 0u) {
        target = runtime.TransformPoint(local_target, target_transform);
    } else {
        auto matrix = runtime.BuildTransformMatrix(target_transform);
        runtime.MultiplyMatrix(matrix, source_frame + 16u);
        target = runtime.TransformPoint(local_target, matrix);
    }

    const auto delta = Subtract(target, origin);
    const float length = runtime.VectorLength(delta);
    state.words[30u] = std::bit_cast<std::uint32_t>(length);
    const auto direction = runtime.Normalize(delta);
    const double half_length = static_cast<double>(length) * 0.5;
    ThunderVector3 midpoint{};
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        midpoint[i] = static_cast<float>(
            static_cast<double>(direction[i]) * half_length + origin[i]);
    }
    WriteVector(state, 64u, midpoint);

    const auto up = runtime.Normalize(direction);
    const auto camera_transform = runtime.FrameTransform(runtime.CameraFrame());
    const auto camera_position = runtime.MatrixPosition(camera_transform);
    const auto at = runtime.Normalize(Subtract(camera_position, origin));
    const auto right = runtime.Normalize(Cross(up, at));
    ThunderFrameMatrix matrix{};
    matrix.right = right;
    matrix.flags = 0x20003u;
    matrix.up = up;
    matrix.at = at;
    runtime.MatrixUpdate(matrix);

    const auto output_frame = state.words[34u];
    runtime.FrameSetIdentity(output_frame);
    runtime.ApplyMatrix(output_frame, matrix, 0u);
    return runtime.FrameUpdateObjects(output_frame);
}

}  // namespace pal4
