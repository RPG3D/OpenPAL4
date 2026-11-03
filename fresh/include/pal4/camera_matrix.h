#pragma once

#include "pal4/camera_data.h"

#include <array>
#include <cstdint>
#include <optional>

namespace pal4 {

class CameraBounds;

class ICameraMatrixRuntime {
public:
    virtual ~ICameraMatrixRuntime() = default;
    virtual void Rotate(CameraDataState& state,
                        std::uint32_t matrix_offset,
                        const std::array<float, 3>& axis,
                        float degrees, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CameraFrame(
        std::uint32_t camera) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ApplyTransform(std::uint32_t frame,
                                CameraDataState& state,
                                std::uint32_t matrix_offset,
                                std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
};

class CameraMatrix final {
public:
    CameraMatrix(ICameraMatrixRuntime& runtime, CameraBounds& bounds)
        : runtime_(runtime), bounds_(bounds) {}

    // 0x5ea070: initialize the secondary matrix at +0x128.
    [[nodiscard]] std::uint32_t* Initialize(CameraDataState& state);
    // 0x5ea190: solve eye or target from the rotated main matrix.
    [[nodiscard]] std::optional<std::uint32_t> Update(
        CameraDataState& state, std::uint32_t solve_eye);

private:
    ICameraMatrixRuntime& runtime_;
    CameraBounds& bounds_;
};

}  // namespace pal4
