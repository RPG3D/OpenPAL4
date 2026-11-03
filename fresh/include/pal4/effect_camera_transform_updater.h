#pragma once

#include "pal4/effect_camera_position.h"
#include "pal4/effect_camera_rotation_basis.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectCameraTransformRuntime : public IEffectCameraPositionRuntime,
                                      public IEffectCameraBasisRuntime {
public:
    virtual ~IEffectCameraTransformRuntime() = default;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ApplyMatrix(std::uint32_t frame,
                             const CameraMatrixWords& matrix,
                             std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateFrameObjects(
        std::uint32_t frame) = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             const CameraVector3& axis, float angle,
                             std::uint32_t mode) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const std::array<std::uint32_t, 3>& position,
                                std::uint32_t mode) = 0;
};

class EffectCameraTransformRuntimeStub final
    : public IEffectCameraTransformRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] CameraVector3 GetTargetPosition(
        const EffectCameraObjectState&, std::uint32_t) override { return {}; }
    [[nodiscard]] CameraVector3 GetTargetLookAt(
        const EffectCameraObjectState&, std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint32_t ObjectTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    void SetupMatrix(CameraMatrixWords&, std::uint32_t) override {}
    void MultiplyMatrix(CameraMatrixWords&, std::uint32_t) override {}
    void TransformPoint(CameraVector3&,
                        const CameraMatrixWords&) override {}
    void TransformVector(CameraVector3&,
                         const CameraMatrixWords&) override {}
    void Normalize(CameraVector3&) override {}
    void UpdateMatrix(CameraMatrixWords&) override {}
    void SetFrameIdentity(std::uint32_t) override {}
    void ApplyMatrix(std::uint32_t, const CameraMatrixWords&,
                     std::uint32_t) override {}
    [[nodiscard]] std::uint32_t UpdateFrameObjects(
        std::uint32_t) override { return 0u; }
    void RotateFrame(std::uint32_t, const CameraVector3&, float,
                     std::uint32_t) override {}
    void TranslateFrame(std::uint32_t,
                        const std::array<std::uint32_t, 3>&,
                        std::uint32_t) override {}
};

class EffectCameraTransformUpdater final {
public:
    // IDA 0x608870. An absent source frame is the only early null gate.
    [[nodiscard]] static std::uint32_t Update(
        const EffectCameraObjectState& state,
        IEffectCameraTransformRuntime& runtime);
};

}  // namespace pal4
