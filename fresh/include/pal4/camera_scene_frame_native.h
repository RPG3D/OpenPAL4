#pragma once
#include "pal4/camera_scene_frame.h"

namespace pal4 {
// Win32 SDK adapter. Addresses refer to live native camera/clump objects;
// callback addresses use the original cdecl one-argument contract.
class NativeCameraSceneFrameRuntime final : public ICameraSceneFrameRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::int32_t ReadSignedWord(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    float DeviceDepth() override;
    void ClearCamera(std::uint32_t camera,
                     const std::array<std::uint8_t, 4>& color,
                     std::uint32_t flags) override;
    std::uint32_t BeginUpdate(std::uint32_t camera) override;
    void InvokeCallback(std::uint32_t callback, std::uint32_t argument) override;
    void RenderClump(std::uint32_t clump) override;
    void EndUpdate(std::uint32_t camera) override;
    std::uint32_t SetDeviceState(std::uint32_t state, std::uint32_t value) override;
    void DrawScreenQuad(std::uint32_t primitive,
                        const std::array<CameraScreenVertex, 4>& vertices,
                        std::uint32_t count) override;
};
}
