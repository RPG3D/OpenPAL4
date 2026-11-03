#pragma once
#include "pal4/camera_resource_helpers.h"

namespace pal4 {
// Native Win32 SDK handles only; these are never original-process addresses.
// Engine must be started, and game-owned camera resources destroyed before Stop.
class NativeCameraResourceRuntime final : public ICameraResourceRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t CreateCamera() override;
    std::uint32_t CreateFrame() override;
    void AttachFrame(std::uint32_t camera, std::uint32_t frame) override;
    std::uint32_t CreateRaster(std::uint32_t width, std::uint32_t height,
                               std::uint32_t depth, std::uint32_t type) override;
    void SetProjection(std::uint32_t camera, std::uint32_t type) override;
    void DestroyFrame(std::uint32_t frame) override;
    void DestroyRaster(std::uint32_t raster) override;
    std::uint32_t DestroyCamera(std::uint32_t camera) override;
    void SetFarClip(std::uint32_t camera, float value) override;
    void SetNearClip(std::uint32_t camera, float value) override;
    void SetViewWindow(std::uint32_t camera, float width, float height) override;
    void CopyMatrix(std::uint32_t destination, std::uint32_t source) override;
    void UpdateFrame(std::uint32_t frame) override;
    void TranslateFrame(std::uint32_t frame, const CameraRenderVector3& offset,
                        std::uint32_t mode) override;
};
}
