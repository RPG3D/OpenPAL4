#include "pal4/game_manager_render_parameters.h"

namespace pal4 {

void GameManagerRenderParameterRuntimeStub::SyncCamera(
    std::uint32_t /*camera_record*/, std::uint32_t /*first*/,
    std::uint32_t /*second*/) {}
std::uint32_t GameManagerRenderParameterRuntimeStub::InvokeHandler(
    std::uint32_t /*handler*/, std::uint32_t /*slot_offset*/,
    std::uint32_t /*first*/, std::uint32_t /*second*/) {
    return 0u;
}

std::uint32_t GameManagerRenderParameters::Update(
    const ScreenEffectDispatcherState& state, const std::uint32_t first,
    const std::uint32_t second, const std::uint32_t incoming_eax,
    IGameManagerRenderParameterRuntime& runtime) {
    if (state.enabled == 0u) {
        return incoming_eax;
    }
    if (state.active <= 0) {
        return static_cast<std::uint32_t>(state.active);
    }
    runtime.SyncCamera(state.frame_state, first, second);
    if (state.current_key == 0) {
        return 0u;
    }
    return runtime.InvokeHandler(state.current_handler, 0x38u,
                                 first, second);
}

}  // namespace pal4
