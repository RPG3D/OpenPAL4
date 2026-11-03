#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct CameraTextureVertex final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float reciprocal_w = 0.0f;
    std::uint32_t color = 0xffffffffu;
    float u = 0.0f;
    float v = 0.0f;
};

static_assert(sizeof(CameraTextureVertex) == 28);

class ICameraTexturePassRuntime {
public:
    virtual ~ICameraTexturePassRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t type) = 0;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t BeginUpdate(
        std::uint32_t camera) = 0;
    virtual void EndUpdate(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t SetDeviceState(
        std::uint32_t state, std::uint32_t value) = 0;
    [[nodiscard]] virtual float DeviceDepth() = 0;
    virtual void DrawTexturedQuad(
        std::uint32_t primitive,
        const std::array<CameraTextureVertex, 4u>& vertices,
        std::uint32_t count) = 0;
};

class CameraTexturePassRuntimeStub final : public ICameraTexturePassRuntime {
public:
    [[nodiscard]] std::uint32_t CreateRaster(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::int32_t ReadSignedWord(
        std::uint32_t) override { return 0; }
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 1.0f; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t BeginUpdate(
        std::uint32_t) override { return 0u; }
    void EndUpdate(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SetDeviceState(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] float DeviceDepth() override { return 0.0f; }
    void DrawTexturedQuad(
        std::uint32_t,
        const std::array<CameraTextureVertex, 4u>&,
        std::uint32_t) override {}
};

class CameraTexturePass final {
public:
    // IDA 0x5fda40.
    [[nodiscard]] static std::uint32_t CreateSquareRaster(
        std::uint32_t size, ICameraTexturePassRuntime& runtime);
    // IDA 0x5fda60.
    [[nodiscard]] static std::uint32_t RenderToTexture(
        std::uint32_t raster, std::uint32_t texture,
        std::uint32_t camera, ICameraTexturePassRuntime& runtime);
    // IDA 0x5fdc80.
    [[nodiscard]] static std::uint32_t RenderToTextureRepeated(
        std::uint32_t first_raster, std::uint32_t second_raster,
        std::uint32_t camera, std::uint32_t iterations,
        ICameraTexturePassRuntime& runtime);
    // IDA 0x5fdb60.
    [[nodiscard]] static std::uint32_t DrawProjectionQuad(
        float left, float top, float right, float bottom,
        float depth, float reciprocal_w, float uv_offset,
        ICameraTexturePassRuntime& runtime);
};

}  // namespace pal4
