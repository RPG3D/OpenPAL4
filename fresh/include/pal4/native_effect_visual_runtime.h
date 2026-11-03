#pragma once

#include "pal4/effect_visual_object.h"
#include "pal4/geo_plane.h"
#include "pal4/native_visual_model_runtime.h"

namespace pal4 {
class NativeBaseObjectMemory;

// SDK boundary for the shared billboard visual and geoPlane algorithms.
class NativeEffectVisualRuntime final : public IEffectVisualObjectRuntime {
public:
    explicit NativeEffectVisualRuntime(NativeBaseObjectMemory& memory) : memory_(memory) {}
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    void WriteGeometryByte(std::uint32_t, std::uint8_t) override;
    void WriteGeometryWord(std::uint32_t, std::uint32_t) override;
    std::uint32_t ReadGeometryWord(std::uint32_t) override;
    bool InitializeGeometry(std::uint32_t, std::uint32_t, std::uint32_t,
                            std::uint8_t, std::uint32_t) override;
    void DestroyGeometry(std::uint32_t) override;
    std::uint32_t CreateFrame() override;
    void DestroyFrame(std::uint32_t) override;
    void SetFrameIdentity(std::uint32_t) override;
    void ScaleFrame(std::uint32_t, const std::array<std::uint32_t, 3>&, std::uint32_t) override;
    void RotateFrame(std::uint32_t, std::uint32_t, float, std::uint32_t) override;
    void TranslateFrame(std::uint32_t, const std::array<std::uint32_t, 3>&, std::uint32_t) override;
    std::uint8_t UpdateFrameObjects(std::uint32_t) override;
    void AddFrameChild(std::uint32_t, std::uint32_t) override;
    std::int32_t SetGeometryState(std::uint32_t, std::int32_t) override;
    void NoopUpdate(std::uint32_t, std::uint32_t) override;
    std::uint32_t ReadObjectWord(std::uint32_t) override;
    EffectMatrix GetObjectTransform(std::uint32_t) override;
    void Normalize(EffectVector3&) override;
    EffectMatrix SetupParentTransform(std::uint32_t) override;
    void TransformVector(EffectVector3&, const EffectMatrix&) override;
    void UpdateMatrix(EffectMatrix&) override;
    void ApplyMatrix(std::uint32_t, const EffectMatrix&, std::uint32_t) override;
    std::uint8_t RenderWithStates(std::uint32_t, std::uint32_t, std::uint32_t) override;

    void SetPlaneColor(std::uint32_t plane, std::uint32_t rgba);
    void SetPlanePosition(std::uint32_t, GeoPlaneVector);
    void SetPlaneRotation(std::uint32_t, GeoPlaneVector);
    void SetPlaneScale(std::uint32_t, GeoPlaneVector);
    void UpdatePlaneUV(std::uint32_t, const std::array<float,4>&);
    void SetPlaneVertexOrientation(std::uint32_t, bool reversed);
private:
    NativeBaseObjectMemory& memory_;
    NativeVisualModelRuntime models_;
};
} // namespace pal4
