#pragma once

#include "pal4/effect_camera_geometry.h"
#include "pal4/effect_camera_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectCameraBasisRuntime {
public:
    virtual ~IEffectCameraBasisRuntime() = default;
    virtual void Normalize(CameraVector3& vector) = 0;
    virtual void UpdateMatrix(CameraMatrixWords& matrix) = 0;
};

class IEffectCameraRotationBasisRuntime : public IEffectCameraBasisRuntime {
public:
    virtual ~IEffectCameraRotationBasisRuntime() = default;
    [[nodiscard]] virtual std::array<std::uint32_t, 3> CalculateAngles(
        const CameraMatrixWords& matrix) = 0;
    [[nodiscard]] virtual std::uint8_t ApplyAngles(
        EffectCameraObjectState& state,
        const std::array<std::uint32_t, 3>& angles,
        std::uint32_t mode) = 0;
};

class EffectCameraRotationBasisRuntimeStub final
    : public IEffectCameraRotationBasisRuntime {
public:
    void Normalize(CameraVector3&) override {}
    void UpdateMatrix(CameraMatrixWords&) override {}
    [[nodiscard]] std::array<std::uint32_t, 3> CalculateAngles(
        const CameraMatrixWords&) override { return {}; }
    [[nodiscard]] std::uint8_t ApplyAngles(
        EffectCameraObjectState&,
        const std::array<std::uint32_t, 3>&,
        std::uint32_t) override { return 0u; }
};

class EffectCameraRotationBasis final {
public:
    // Shared by IDA 0x608200 and 0x608870; no object-state gate here.
    [[nodiscard]] static CameraMatrixWords BuildMatrixFromDirection(
        CameraVector3 direction, IEffectCameraBasisRuntime& runtime);
    // IDA 0x608200, through the facet at owner +20.
    [[nodiscard]] static std::uint8_t SetFromDirection(
        EffectCameraObjectState& state, CameraVector3 direction,
        IEffectCameraRotationBasisRuntime& runtime);
    // IDA 0x6083e0, through the same facet.
    [[nodiscard]] static std::uint8_t SetProperties(
        EffectCameraObjectState& state,
        const std::array<std::uint32_t, 3>& values);
};

}  // namespace pal4
