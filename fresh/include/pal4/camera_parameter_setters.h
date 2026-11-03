#pragma once

#include "pal4/camera_data.h"

#include <array>
#include <cstdint>
#include <optional>

namespace pal4 {

class ICameraParameterRuntime {
public:
    virtual ~ICameraParameterRuntime() = default;
    [[nodiscard]] virtual float MinimumDistance() = 0;
    [[nodiscard]] virtual float MaximumDistance() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateMatrix(
        CameraDataState& state, std::uint32_t mode) = 0;
};

class CameraParameterSetters final {
public:
    explicit CameraParameterSetters(ICameraParameterRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e9c30: write eye XYZ at +0x68.
    [[nodiscard]] std::optional<std::uint32_t> SetEye(
        CameraDataState& state, const std::array<float, 3>& eye,
        std::uint32_t mode);
    // 0x5e9c60: copy target XYZ from a vector at +0x74.
    [[nodiscard]] std::optional<std::uint32_t> SetLookAt(
        CameraDataState& state, const std::array<float, 3>& target,
        std::uint32_t mode);
    // 0x5e9c90: write three individual target XYZ values.
    [[nodiscard]] std::optional<std::uint32_t> SetTargetXyz(
        CameraDataState& state, float x, float y, float z,
        std::uint32_t mode);
    // 0x5e9cc0: clamp distance and mirror it to current distance.
    [[nodiscard]] std::optional<std::uint32_t> SetDistance(
        CameraDataState& state, float distance, std::uint32_t mode);
    // 0x5e9d20/0x5e9d70/0x5e9dc0: set one normalized angle.
    [[nodiscard]] std::uint32_t SetFirstAngle(
        CameraDataState& state, float angle, std::uint32_t mode);
    [[nodiscard]] std::uint32_t SetSecondAngle(
        CameraDataState& state, float angle, std::uint32_t mode);
    [[nodiscard]] std::uint32_t SetThirdAngle(
        CameraDataState& state, float angle, std::uint32_t mode);
    // 0x5e9e10: normalize all three angles.
    [[nodiscard]] std::uint32_t SetAllAngles(
        CameraDataState& state, const std::array<float, 3>& angles,
        std::uint32_t mode);
    // Shared 0x5eace0 operation, also used by the script's active-camera adapter.
    [[nodiscard]] std::uint32_t AdjustAngles(
        CameraDataState& state, const std::array<float, 3>& delta,
        std::uint32_t mode);
    // 0x5e9ef0: normalize angles, clamp target distance and retain raw current.
    [[nodiscard]] std::uint32_t SetAnglesAndDistance(
        CameraDataState& state, const std::array<float, 3>& angles,
        float distance, std::uint32_t mode);
    // 0x5ea020: initialize five camera parameter slots.
    [[nodiscard]] std::uint32_t InitializeParameters(
        CameraDataState& state, std::uint32_t first,
        std::uint32_t second, std::uint32_t third);
    // 0x5ea050: set active state and snapshot target Y.
    [[nodiscard]] std::uint32_t SetActiveState(
        CameraDataState& state, std::uint32_t active);

private:
    [[nodiscard]] static float NormalizeAngle(float angle);
    [[nodiscard]] float ClampDistance(float distance);
    [[nodiscard]] std::uint32_t SetAngle(
        CameraDataState& state, std::size_t offset,
        float angle, std::uint32_t mode);

    ICameraParameterRuntime& runtime_;
};

}  // namespace pal4
