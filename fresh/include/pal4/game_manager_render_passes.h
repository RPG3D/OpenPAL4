#pragma once

#include "pal4/screen_effect_dispatcher.h"

#include <cstdint>

namespace pal4 {

struct GameRenderDimensions {
    std::int32_t width{};
    std::int32_t height{};
};

class IGameManagerRenderPassRuntime {
public:
    virtual ~IGameManagerRenderPassRuntime() = default;
    [[nodiscard]] virtual std::int32_t PassCount(
        std::uint32_t handler, std::uint32_t frame_state) = 0;
    virtual void SelectPass(std::uint32_t handler,
                            std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint8_t RenderPass(
        std::uint32_t handler, std::uint32_t argument,
        const ScreenEffectDispatcherState& state) = 0;
    virtual void FinishPass(std::uint32_t handler) = 0;
    virtual void EndPasses(std::uint32_t handler) = 0;
    [[nodiscard]] virtual GameRenderDimensions ScreenDimensions() = 0;
    [[nodiscard]] virtual std::uint32_t FrameTexture(
        std::uint32_t frame_state) = 0;
    virtual void DrawQuad(std::uint32_t texture, float x, float y,
                          float width, float height,
                          std::int32_t color) = 0;
};

class GameManagerRenderPassRuntimeStub final
    : public IGameManagerRenderPassRuntime {
public:
    [[nodiscard]] std::int32_t PassCount(
        std::uint32_t handler, std::uint32_t frame_state) override;
    void SelectPass(std::uint32_t handler, std::int32_t index) override;
    [[nodiscard]] std::uint8_t RenderPass(
        std::uint32_t handler, std::uint32_t argument,
        const ScreenEffectDispatcherState& state) override;
    void FinishPass(std::uint32_t handler) override;
    void EndPasses(std::uint32_t handler) override;
    [[nodiscard]] GameRenderDimensions ScreenDimensions() override;
    [[nodiscard]] std::uint32_t FrameTexture(
        std::uint32_t frame_state) override;
    void DrawQuad(std::uint32_t texture, float x, float y,
                  float width, float height,
                  std::int32_t color) override;
};

class GameManagerRenderPasses final {
public:
    static void Render(const ScreenEffectDispatcherState& state,
                       std::uint32_t argument,
                       IGameManagerRenderPassRuntime& runtime);
};

}  // namespace pal4
