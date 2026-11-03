#pragma once

#include "pal4/camera_render_object.h"

namespace pal4 {
struct BaseObjectState;

// Original game cleanup order, using public SDK destruction and the existing
// CameraResourceHelpers. Caller owns the embedded image and live SDK handles.
class NativeCameraRenderCleanup final : public ICameraRenderCleanupRuntime {
public:
    // IDA666520 -> 5FC930; embedded CameraRenderState starts at resource+0x38.
    static void CleanupResource(BaseObjectState& resource);
    void DetachTextureRaster(std::uint32_t texture) override;
    void DestroyTexture(std::uint32_t texture) override;
    void DestroyPipeline(std::uint32_t pipeline) override;
    std::uint32_t GetCameraWorld(std::uint32_t camera) override;
    void RemoveFromWorld(std::uint32_t world, std::uint32_t camera) override;
    void DestroyCamera(std::uint32_t camera) override;
    std::uint32_t DestroyRaster(std::uint32_t raster) override;
};
}  // namespace pal4
