#pragma once

#include "pal4/effect_camera_object.h"
#include "pal4/effect_camera_geometry.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectCameraPositionRuntime {
public:
    virtual ~IEffectCameraPositionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual CameraVector3 GetTargetPosition(
        const EffectCameraObjectState& state,
        std::uint32_t name_offset) = 0;
    [[nodiscard]] virtual CameraVector3 GetTargetLookAt(
        const EffectCameraObjectState& state,
        std::uint32_t name_offset) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void SetupMatrix(CameraMatrixWords& matrix,
                             std::uint32_t transform) = 0;
    virtual void MultiplyMatrix(CameraMatrixWords& matrix,
                                std::uint32_t frame_matrix) = 0;
    virtual void TransformPoint(CameraVector3& point,
                                const CameraMatrixWords& matrix) = 0;
    virtual void TransformVector(CameraVector3& vector,
                                 const CameraMatrixWords& matrix) = 0;
};

class EffectCameraPositionRuntimeStub final
    : public IEffectCameraPositionRuntime {
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
};

class EffectCameraPosition final {
public:
    // Shared by IDA 0x607ff0 and 0x608870; no object-state gate here.
    [[nodiscard]] static CameraVector3 ResolveLookAt(
        const EffectCameraObjectState& state,
        IEffectCameraPositionRuntime& runtime);
    // IDA 0x607ff0. Native AL contains the third output pointer's low byte.
    [[nodiscard]] static std::uint8_t GetLookAtWorld(
        const EffectCameraObjectState& state,
        std::array<std::uint32_t, 3>& output,
        std::uint8_t third_output_address_low_byte,
        IEffectCameraPositionRuntime& runtime);
};

}  // namespace pal4
