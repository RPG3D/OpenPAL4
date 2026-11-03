#pragma once
#include "pal4/camera_render_object.h"
#include "pal4/native_camera_render_cleanup.h"
#include <rwcore.h>
#include <rpworld.h>

namespace pal4 {
// The caller supplies the typed implementation of game callback 5FD300.
// No original-process addresses are executed, and no fallback render is installed.
class NativeCameraRenderLifecycleRuntime final : public ICameraRenderLifecycleRuntime {
public:
    explicit NativeCameraRenderLifecycleRuntime(RxD3D9AllInOneRenderCallBack render);
    float ReadFloat(std::uint32_t address) override;
    std::uint32_t CreateCamera(std::uint32_t size) override;
    void SetFrame(std::uint32_t camera, std::uint32_t source) override;
    void SetClip(std::uint32_t camera, float size) override;
    void UpdateTarget(std::uint32_t camera, std::uint32_t target) override;
    void AddToWorld(std::uint32_t world, std::uint32_t camera) override;
    std::uint32_t CreateRaster(std::uint32_t size) override;
    void SetCameraRaster(std::uint32_t camera, std::uint32_t raster) override;
    std::uint32_t CreatePipeline() override;
    std::uint32_t CreateTexture(std::uint32_t raster) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void DetachTextureRaster(std::uint32_t texture) override;
    void DestroyTexture(std::uint32_t texture) override;
    void DestroyPipeline(std::uint32_t pipeline) override;
    std::uint32_t GetCameraWorld(std::uint32_t camera) override;
    void RemoveFromWorld(std::uint32_t world, std::uint32_t camera) override;
    void DestroyCamera(std::uint32_t camera) override;
    std::uint32_t DestroyRaster(std::uint32_t raster) override;
private:
    RxD3D9AllInOneRenderCallBack render_;
    NativeCameraRenderCleanup cleanup_;
};
}
