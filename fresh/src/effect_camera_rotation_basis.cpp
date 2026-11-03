#include "pal4/effect_camera_rotation_basis.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t ReadByte(const EffectCameraObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

CameraVector3 Cross(const CameraVector3& left,
                    const CameraVector3& right) {
    return {left[1u] * right[2u] - left[2u] * right[1u],
            left[2u] * right[0u] - left[0u] * right[2u],
            left[0u] * right[1u] - left[1u] * right[0u]};
}

void StoreVector(CameraMatrixWords& matrix, const std::uint32_t offset,
                 const CameraVector3& vector) {
    for (std::uint32_t index = 0u; index < vector.size(); ++index)
        matrix[offset + index] = std::bit_cast<std::uint32_t>(vector[index]);
}

}  // namespace

CameraMatrixWords EffectCameraRotationBasis::BuildMatrixFromDirection(
    CameraVector3 direction, IEffectCameraBasisRuntime& runtime) {
    CameraVector3 up{0.0f, 1.0f, 0.0f};
    runtime.Normalize(direction);
    CameraVector3 right = Cross(up, direction);
    runtime.Normalize(right);
    up = Cross(direction, right);
    runtime.Normalize(up);

    CameraMatrixWords matrix{};
    StoreVector(matrix, 0u, right);
    matrix[3u] |= 0x20003u;
    StoreVector(matrix, 4u, up);
    StoreVector(matrix, 8u, direction);
    matrix[12u] = matrix[13u] = matrix[14u] = 0u;
    runtime.UpdateMatrix(matrix);
    return matrix;
}

std::uint8_t EffectCameraRotationBasis::SetFromDirection(
    EffectCameraObjectState& state, CameraVector3 direction,
    IEffectCameraRotationBasisRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u || ReadByte(state, 80u) == 0u)
        return 0u;
    const auto matrix = BuildMatrixFromDirection(direction, runtime);
    const auto angles = runtime.CalculateAngles(matrix);
    return runtime.ApplyAngles(state, angles, 0u);
}

std::uint8_t EffectCameraRotationBasis::SetProperties(
    EffectCameraObjectState& state,
    const std::array<std::uint32_t, 3>& values) {
    if (ReadByte(state, 4u) == 0u || ReadByte(state, 80u) == 0u)
        return 0u;
    for (std::uint32_t index = 0u; index < values.size(); ++index)
        state.words[25u + index] = values[index];
    return static_cast<std::uint8_t>(values[2u]);
}

}  // namespace pal4
