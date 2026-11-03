#include "pal4/camera_parameter_setters.h"

#include <bit>

namespace pal4 {

std::uint32_t CameraParameterSetters::AdjustAngles(
    CameraDataState& state, const std::array<float, 3>& delta,
    std::uint32_t mode) {
    std::array<float, 3> angles{};
    for (std::size_t axis = 0; axis < angles.size(); ++axis)
        angles[axis] = std::bit_cast<float>(state.At(0x3cu + axis * 4u)) + delta[axis];
    return SetAllAngles(state, angles, mode);
}

std::optional<std::uint32_t> CameraParameterSetters::SetEye(
    CameraDataState& state, const std::array<float, 3>& eye,
    std::uint32_t mode) {
    if (state.At(0) == 0u) return std::nullopt;
    for (std::size_t i = 0; i < eye.size(); ++i)
        state.At(0x68u + i * 4u) = std::bit_cast<std::uint32_t>(eye[i]);
    return runtime_.UpdateMatrix(state, mode);
}

std::optional<std::uint32_t> CameraParameterSetters::SetLookAt(
    CameraDataState& state, const std::array<float, 3>& target,
    std::uint32_t mode) {
    if (state.At(0) == 0u) return std::nullopt;
    for (std::size_t i = 0; i < target.size(); ++i)
        state.At(0x74u + i * 4u) = std::bit_cast<std::uint32_t>(target[i]);
    return runtime_.UpdateMatrix(state, mode);
}

std::optional<std::uint32_t> CameraParameterSetters::SetTargetXyz(
    CameraDataState& state, float x, float y, float z,
    std::uint32_t mode) {
    return SetLookAt(state, {x, y, z}, mode);
}

std::optional<std::uint32_t> CameraParameterSetters::SetDistance(
    CameraDataState& state, float distance, std::uint32_t mode) {
    if (state.At(0) == 0u) return std::nullopt;
    const auto clamped = ClampDistance(distance);
    state.At(0x4cu) = std::bit_cast<std::uint32_t>(clamped);
    state.At(0x48u) = state.At(0x4cu);
    return runtime_.UpdateMatrix(state, mode);
}

std::uint32_t CameraParameterSetters::SetFirstAngle(
    CameraDataState& state, float angle, std::uint32_t mode) {
    return SetAngle(state, 0x3cu, angle, mode);
}

std::uint32_t CameraParameterSetters::SetSecondAngle(
    CameraDataState& state, float angle, std::uint32_t mode) {
    return SetAngle(state, 0x40u, angle, mode);
}

std::uint32_t CameraParameterSetters::SetThirdAngle(
    CameraDataState& state, float angle, std::uint32_t mode) {
    return SetAngle(state, 0x44u, angle, mode);
}

std::uint32_t CameraParameterSetters::SetAllAngles(
    CameraDataState& state, const std::array<float, 3>& angles,
    std::uint32_t mode) {
    for (std::size_t i = 0; i < angles.size(); ++i)
        state.At(0x3cu + i * 4u) = std::bit_cast<std::uint32_t>(
            NormalizeAngle(angles[i]));
    return runtime_.UpdateMatrix(state, mode);
}

std::uint32_t CameraParameterSetters::SetAnglesAndDistance(
    CameraDataState& state, const std::array<float, 3>& angles,
    float distance, std::uint32_t mode) {
    for (std::size_t i = 0; i < angles.size(); ++i)
        state.At(0x3cu + i * 4u) = std::bit_cast<std::uint32_t>(
            NormalizeAngle(angles[i]));
    state.At(0x4cu) = std::bit_cast<std::uint32_t>(
        ClampDistance(distance));
    state.At(0x48u) = std::bit_cast<std::uint32_t>(distance);
    return runtime_.UpdateMatrix(state, mode);
}

std::uint32_t CameraParameterSetters::InitializeParameters(
    CameraDataState& state, std::uint32_t first,
    std::uint32_t second, std::uint32_t third) {
    state.At(0x94u) = first;
    state.At(0x98u) = second;
    state.At(0x90u) = 0u;
    state.At(0x9cu) = third;
    state.At(0xa0u) = 0u;
    return 0u;
}

std::uint32_t CameraParameterSetters::SetActiveState(
    CameraDataState& state, std::uint32_t active) {
    state.At(0x8cu) = active;
    state.At(0xa4u) = state.At(0x78u);
    return active;
}

float CameraParameterSetters::NormalizeAngle(float angle) {
    if (angle < 0.0f) angle += 360.0f;
    if (angle >= 360.0f) angle -= 360.0f;
    return angle;
}

float CameraParameterSetters::ClampDistance(float distance) {
    const auto minimum = runtime_.MinimumDistance();
    if (distance < minimum) distance = minimum;
    const auto maximum = runtime_.MaximumDistance();
    if (distance >= maximum) distance = maximum;
    return distance;
}

std::uint32_t CameraParameterSetters::SetAngle(
    CameraDataState& state, std::size_t offset,
    float angle, std::uint32_t mode) {
    state.At(offset) = std::bit_cast<std::uint32_t>(NormalizeAngle(angle));
    return runtime_.UpdateMatrix(state, mode);
}

}  // namespace pal4
