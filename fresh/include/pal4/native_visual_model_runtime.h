#pragma once
#include "pal4/visual_model.h"
#include "pal4/geo_cylinder.h"

namespace pal4 {
// SDK operations only; the shared VisualModel/GeoCylinder implementations
// retain transform ordering, mesh generation and render-state policy.
class NativeVisualModelRuntime final : public IVisualModelRuntime,
    public IVisualModelDestroyRuntime, public IVisualModelRenderRuntime {
public:
    void CreateCylinder(VisualModelState& model, const CylinderCreateOptions& options);
    void SetFrameIdentity(std::uint32_t frame) override;
    void ScaleFrame(std::uint32_t frame, VisualModelVector value, std::uint32_t mode) override;
    void RotateFrame(std::uint32_t frame, std::uint32_t axis, float angle, std::uint32_t mode) override;
    void TranslateFrame(std::uint32_t frame, VisualModelVector value, std::uint32_t mode) override;
    int UpdateFrameObjects(std::uint32_t frame) override;
    int SetMaterialFlagsForAllAtomics(std::uint32_t clump, std::uint32_t color) override;
    std::uint32_t FindClumpWorld(std::uint32_t clump) override;
    void RemoveClumpFromWorld(std::uint32_t world, std::uint32_t clump) override;
    void ClearClumpFrameLink(std::uint32_t clump) override;
    void DestroyClump(std::uint32_t clump) override;
    void DestroyFrame(std::uint32_t frame) override;
    std::uint32_t GetRenderState(std::uint32_t state) override;
    void SetRenderState(std::uint32_t state, std::uint32_t value) override;
    std::uint32_t GetDeviceState(std::uint32_t state) override;
    void SetDeviceState(std::uint32_t state, std::uint32_t value) override;
    void RenderClump(std::uint32_t clump) override;
    void SetTextureStage(std::uint32_t first, std::uint32_t second, std::uint32_t stage) override;
    int ClearStreamSource(std::uint32_t stream) override;
};
} // namespace pal4
