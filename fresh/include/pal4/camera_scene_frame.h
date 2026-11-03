#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct CameraScreenVertex final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float reciprocal_w = 0.0f;
    std::uint32_t color = 0u;
    std::uint32_t texture_u = 0u;
    std::uint32_t texture_v = 0u;
};

static_assert(sizeof(CameraScreenVertex) == 28);

class ICameraSceneFrameRuntime {
public:
    virtual ~ICameraSceneFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual float DeviceDepth() = 0;
    virtual void ClearCamera(std::uint32_t camera,
                             const std::array<std::uint8_t, 4u>& color,
                             std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t BeginUpdate(
        std::uint32_t camera) = 0;
    virtual void InvokeCallback(std::uint32_t callback,
                                std::uint32_t argument) = 0;
    virtual void RenderClump(std::uint32_t clump) = 0;
    virtual void EndUpdate(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t SetDeviceState(
        std::uint32_t state, std::uint32_t value) = 0;
    virtual void DrawScreenQuad(
        std::uint32_t primitive,
        const std::array<CameraScreenVertex, 4u>& vertices,
        std::uint32_t count) = 0;
};

class CameraSceneFrame final {
public:
    // IDA 0x5fd880.
    [[nodiscard]] static std::uint32_t RenderInternal(
        std::uint32_t camera, std::uint32_t clump,
        bool draw_quad, std::uint32_t callback,
        std::uint32_t callback_argument,
        ICameraSceneFrameRuntime& runtime);
    // IDA 0x5fd900.
    [[nodiscard]] static std::uint32_t DrawQuad(
        std::uint32_t camera, ICameraSceneFrameRuntime& runtime);
};

}  // namespace pal4
