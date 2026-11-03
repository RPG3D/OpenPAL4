#pragma once
#include "pal4/camera_texture_pass.h"

namespace pal4 {
// SDK boundary for the shared IDA 5FDA60/5FDC80 texture passes.
// State 1 receives an RwRaster pointer, despite the original parameter name.
class NativeCameraTexturePassRuntime final : public ICameraTexturePassRuntime {
public:
    std::uint32_t CreateRaster(std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t type) override;
    std::int32_t ReadSignedWord(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t BeginUpdate(std::uint32_t camera) override;
    void EndUpdate(std::uint32_t camera) override;
    std::uint32_t SetDeviceState(std::uint32_t state, std::uint32_t value) override;
    float DeviceDepth() override;
    void DrawTexturedQuad(std::uint32_t primitive,
        const std::array<CameraTextureVertex, 4>& vertices, std::uint32_t count) override;
};
}
