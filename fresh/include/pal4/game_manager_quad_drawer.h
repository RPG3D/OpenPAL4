#pragma once

#include "pal4/cegui_vertex_processor.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IGameManagerQuadRuntime {
public:
    virtual ~IGameManagerQuadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CurrentDevice() = 0;
    [[nodiscard]] virtual std::uint32_t TextureHandle(
        std::uint32_t texture_pointer) = 0;
    virtual void BindTexture(std::uint32_t stage,
                             std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t SetDeviceState(
        std::uint32_t device, std::uint32_t state,
        std::uint32_t value) = 0;
    virtual void SetFvf(std::uint32_t device,
                        std::uint32_t fvf) = 0;
    virtual void DrawPrimitiveUp(
        std::uint32_t device, std::uint32_t primitive,
        std::uint32_t count,
        const std::array<CeguiRenderVertex, 4>& vertices,
        std::uint32_t stride) = 0;
};

class GameManagerQuadRuntimeStub final : public IGameManagerQuadRuntime {
public:
    [[nodiscard]] std::uint32_t CurrentDevice() override;
    [[nodiscard]] std::uint32_t TextureHandle(
        std::uint32_t texture_pointer) override;
    void BindTexture(std::uint32_t stage,
                     std::uint32_t handle) override;
    [[nodiscard]] std::uint32_t SetDeviceState(
        std::uint32_t device, std::uint32_t state,
        std::uint32_t value) override;
    void SetFvf(std::uint32_t device,
                std::uint32_t fvf) override;
    void DrawPrimitiveUp(
        std::uint32_t device, std::uint32_t primitive,
        std::uint32_t count,
        const std::array<CeguiRenderVertex, 4>& vertices,
        std::uint32_t stride) override;
};

class GameManagerQuadDrawer final {
public:
    [[nodiscard]] static std::uint32_t Draw(
        std::uint8_t enabled, std::uint32_t incoming_eax,
        std::uint32_t texture_pointer, float x, float y,
        float width, float height, std::uint32_t color,
        IGameManagerQuadRuntime& runtime);
};

}  // namespace pal4
