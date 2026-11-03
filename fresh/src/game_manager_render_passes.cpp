#include "pal4/game_manager_render_passes.h"

namespace pal4 {
namespace {

void DrawFallback(const ScreenEffectDispatcherState& state,
                  IGameManagerRenderPassRuntime& runtime) {
    const auto dimensions = runtime.ScreenDimensions();
    const auto texture = runtime.FrameTexture(state.frame_state);
    runtime.DrawQuad(texture, 0.0f, 0.0f,
                     static_cast<float>(dimensions.width),
                     static_cast<float>(dimensions.height), -1);
}

}  // namespace

std::int32_t GameManagerRenderPassRuntimeStub::PassCount(
    std::uint32_t /*handler*/, std::uint32_t /*frame_state*/) { return 0; }
void GameManagerRenderPassRuntimeStub::SelectPass(
    std::uint32_t /*handler*/, std::int32_t /*index*/) {}
std::uint8_t GameManagerRenderPassRuntimeStub::RenderPass(
    std::uint32_t /*handler*/, std::uint32_t /*argument*/,
    const ScreenEffectDispatcherState& /*state*/) { return 0u; }
void GameManagerRenderPassRuntimeStub::FinishPass(
    std::uint32_t /*handler*/) {}
void GameManagerRenderPassRuntimeStub::EndPasses(
    std::uint32_t /*handler*/) {}
GameRenderDimensions GameManagerRenderPassRuntimeStub::ScreenDimensions() {
    return {};
}
std::uint32_t GameManagerRenderPassRuntimeStub::FrameTexture(
    std::uint32_t /*frame_state*/) { return 0u; }
void GameManagerRenderPassRuntimeStub::DrawQuad(
    std::uint32_t /*texture*/, float /*x*/, float /*y*/,
    float /*width*/, float /*height*/, std::int32_t /*color*/) {}

void GameManagerRenderPasses::Render(
    const ScreenEffectDispatcherState& state, const std::uint32_t argument,
    IGameManagerRenderPassRuntime& runtime) {
    if (state.enabled == 0u) {
        return;
    }
    if (state.active > 0 && state.current_key != 0 &&
        state.current_key != 16) {
        const auto count = runtime.PassCount(state.current_handler,
                                             state.frame_state);
        for (std::int32_t index = 0; index < count; ++index) {
            runtime.SelectPass(state.current_handler, index);
            if (runtime.RenderPass(state.current_handler, argument, state) == 0u) {
                DrawFallback(state, runtime);
            }
            runtime.FinishPass(state.current_handler);
        }
        runtime.EndPasses(state.current_handler);
        if (count > 0) {
            return;
        }
    }
    DrawFallback(state, runtime);
}

}  // namespace pal4
