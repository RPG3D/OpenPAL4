#include "pal4/camera_projection_matrix.h"

#include <bit>

namespace pal4 {

std::uint32_t CameraProjectionMatrix::Set(
    const std::uint32_t texture, const std::uint32_t scene,
    const std::uint32_t camera, const float strength, const float depth,
    CameraProjectionGlobals& globals, ICameraProjectionRuntime& runtime) {
    globals.texture = texture;
    globals.strength_bits = std::bit_cast<std::uint32_t>(strength);

    CameraProjectionMatrixImage scene_matrix{};
    const auto scene_frame = runtime.ReadWord(scene + 4u);
    runtime.CopyMatrix(scene_matrix, runtime.GetObjectTransform(scene_frame));
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto value = std::bit_cast<float>(scene_matrix[index]);
        scene_matrix[index] = std::bit_cast<std::uint32_t>(-value);
    }

    const auto camera_frame = runtime.ReadWord(camera + 4u);
    const auto parameter = runtime.ReadFloat(camera + 104u);
    CameraProjectionMatrixImage transform{};
    runtime.InvertMatrix(transform, camera_frame + 16u);
    const CameraProjectionVector3 scale{
        -0.5f / parameter, -0.5f / parameter,
        1.0f / (parameter + depth)};
    runtime.Scale(transform, scale, 2u);
    runtime.Translate(transform, {0.5f, 0.5f, 0.0f}, 2u);

    CameraProjectionMatrixImage result{};
    runtime.Multiply(result, scene_matrix, transform);
    result[3] = 0u;
    result[7] = 0u;
    result[11] = 0u;
    result[15] = 0x3f800000u;
    return runtime.SetTransform(17u, result);
}

}  // namespace pal4
