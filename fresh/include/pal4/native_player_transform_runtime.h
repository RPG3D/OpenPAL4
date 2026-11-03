#pragma once

#include "pal4/game_resource_manager.h"
#include "pal4/base_object_lifecycle.h"

namespace pal4 {

// SDK boundary for actor Clump transforms and the scene render pass.
class NativePlayerTransformRuntime final :
    public GameResourceManagerRuntimeStub {
public:
    [[nodiscard]] bool RenderObjectClump(BaseObjectState& object,
                                         std::uint32_t camera);
    bool FrustumTestSphere(std::uint32_t camera,
                           std::uint32_t* sphere) override;
    void RenderClumpRaw(std::uint32_t clump) override;
    void RenderCameraRaw(std::uint32_t* camera_state,
                         std::uint32_t camera) override;
    void RenderCameraScene(std::uint32_t* camera,float elapsed,std::uint32_t* actor) override;
    std::uint32_t ClumpFrame(std::uint32_t clump) override;
    void FrameSetIdentity(std::uint32_t frame) override;
    void FrameRotate(std::uint32_t frame, std::uint32_t axis_address,
                     float degrees, std::uint32_t combine) override;
    void FrameUpdateObjects(std::uint32_t frame) override;
    void FrameScale(std::uint32_t frame, const float* scale,
                    std::uint32_t combine) override;
    void FrameTranslate(std::uint32_t frame, const float* position,
                        std::uint32_t combine) override;
    std::uint32_t ObjectTransform(std::uint32_t frame) override;
    void ApplyDialogTransform(std::uint32_t frame,
                              std::uint32_t transform,
                              std::uint32_t combine) override;
    void TransformPoints(ResourceCorners& corners,
                         std::uint32_t transform) override;
    void CalculateBBox(std::uint32_t* bbox,
                       const ResourceCorners& corners) override;
    std::uint32_t CalculateCameraPosition(
        std::uint32_t* destination, std::uint32_t* source,
        std::uint32_t frame) override;
    void ProcessAtomicBounds(std::uint32_t clump, std::uint32_t mode,
                             std::uint32_t* bounds) override;
    std::uint32_t ProcessAtomicBounds2(std::uint32_t clump,
                                       std::uint32_t mode,
                                       std::uint32_t initialize,
                                       std::uint32_t* sphere) override;
};

}  // namespace pal4
