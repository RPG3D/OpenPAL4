#pragma once

#include <array>
#include <cstdint>

#include "pal4/cegui_vertex_processor.h"

namespace pal4 {

struct PalRectangleColor {
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
    std::uint8_t alpha{};
};

class IPalRectangleRenderRuntime {
public:
    virtual ~IPalRectangleRenderRuntime() = default;
    [[nodiscard]] virtual float CameraDepth(std::uint32_t context) = 0;
    [[nodiscard]] virtual float ActiveDepth() = 0;
    [[nodiscard]] virtual std::uint32_t TextureRaster(
        std::uint32_t texture) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void DrawPrimitive(
        std::uint32_t primitive,
        const std::array<CeguiRenderVertex, 4>& vertices,
        std::uint32_t count) = 0;
};

class PalRectangleRenderRuntimeStub final : public IPalRectangleRenderRuntime {
public:
    [[nodiscard]] float CameraDepth(std::uint32_t context) override;
    [[nodiscard]] float ActiveDepth() override;
    [[nodiscard]] std::uint32_t TextureRaster(
        std::uint32_t texture) override;
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override;
    void DrawPrimitive(
        std::uint32_t primitive,
        const std::array<CeguiRenderVertex, 4>& vertices,
        std::uint32_t count) override;
};

class PalRectangleDrawer final {
public:
    static constexpr std::uint32_t kRenderStateTable = 0x950cd0u;

    // 0x4198f0. The eight UV words are uninitialized stack bytes in the EXE.
    [[nodiscard]] static std::uint32_t DrawFilled(
        std::uint32_t context, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height,
        PalRectangleColor color,
        const std::array<std::uint32_t, 8>& incoming_uv_bits,
        IPalRectangleRenderRuntime& runtime);
    // 0x419a60.
    [[nodiscard]] static std::uint32_t DrawTextured(
        std::uint32_t context, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height,
        std::uint32_t texture, PalRectangleColor color,
        IPalRectangleRenderRuntime& runtime);
    // 0x419c20.
    [[nodiscard]] static std::uint32_t DrawTexturedWithAlpha(
        std::uint32_t context, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height,
        std::uint32_t texture, PalRectangleColor color,
        IPalRectangleRenderRuntime& runtime);
};

}  // namespace pal4
