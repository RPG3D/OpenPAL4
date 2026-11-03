#pragma once

#include "pal4/effect_camera_geometry.h"
#include "pal4/effect_camera_object.h"

#include <cstdint>

namespace pal4 {

class IEffectCameraConstraintsRuntime {
public:
    virtual ~IEffectCameraConstraintsRuntime() = default;
    virtual void UpdateTransform(EffectCameraObjectState& state) = 0;
    virtual void Translate(std::uint32_t matrix_address,
                           const CameraVector3& offset,
                           std::uint32_t mode) = 0;
    virtual void Rotate(std::uint32_t matrix_address,
                        const CameraVector3& axis, float degrees,
                        std::uint32_t mode) = 0;
    [[nodiscard]] virtual CameraVector3 MatrixTranslation(
        std::uint32_t matrix_address) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        EffectCameraObjectState& state,
        const CameraVector3& position) = 0;
};

class EffectCameraConstraintsRuntimeStub final
    : public IEffectCameraConstraintsRuntime {
public:
    void UpdateTransform(EffectCameraObjectState&) override {}
    void Translate(std::uint32_t, const CameraVector3&,
                   std::uint32_t) override {}
    void Rotate(std::uint32_t, const CameraVector3&, float,
                std::uint32_t) override {}
    [[nodiscard]] CameraVector3 MatrixTranslation(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint32_t SetPosition(
        EffectCameraObjectState&,
        const CameraVector3&) override { return 0u; }
};

class EffectCameraConstraints final {
public:
    // IDA 0x608410, through the facet at owner +28.
    [[nodiscard]] static std::uint32_t Apply(
        EffectCameraObjectState& state, const CameraVector3& pivot,
        const CameraVector3& rotation, const CameraVector3& step,
        IEffectCameraConstraintsRuntime& runtime);
};

}  // namespace pal4
