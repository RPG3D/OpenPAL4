#include "pal4/game_manager_reset.h"

namespace pal4 {

void GameManagerResetRuntimeStub::CallHandler(
    std::uint32_t /*handler*/, std::uint32_t /*slot_offset*/) {}
std::uint32_t GameManagerResetRuntimeStub::FirstNode(
    const std::uint32_t sentinel) { return sentinel; }
std::uint32_t GameManagerResetRuntimeStub::NodeObject(
    std::uint32_t /*node*/) { return 0u; }
void GameManagerResetRuntimeStub::InvokeShutdown(
    std::uint32_t /*object*/, std::uint32_t /*slot_offset*/) {}
void GameManagerResetRuntimeStub::DeleteObject(
    std::uint32_t /*object*/, std::uint8_t /*flags*/) {}
std::uint32_t GameManagerResetRuntimeStub::RemoveNode(
    ScreenEffectDispatcherState& state, std::uint32_t /*node*/) {
    return state.tree_sentinel;
}
void GameManagerResetRuntimeStub::CleanupCamera(
    std::uint32_t /*camera*/) {}
std::uint32_t GameManagerResetRuntimeStub::DestroyCameraRecord(
    std::uint32_t /*camera*/) { return 0u; }

void GameManagerReset::StopActive(
    ScreenEffectDispatcherState& state, IGameManagerResetRuntime& runtime) {
    if (state.enabled == 0u || state.current_key == 0 ||
        state.current_key == 16) {
        return;
    }
    state.current_key = 0;
    const auto handler = state.current_handler;
    runtime.CallHandler(handler, 0x10u);
    runtime.CallHandler(handler, 0x34u);
    state.current_handler = 0u;
    state.active = 0;
}

std::uint32_t GameManagerReset::Reset(
    ScreenEffectDispatcherState& state, const std::uint32_t incoming_eax,
    IGameManagerResetRuntime& runtime) {
    if (state.enabled == 0u) {
        return incoming_eax;
    }
    StopActive(state, runtime);
    const auto sentinel = state.tree_sentinel;
    state.enabled = 0u;
    state.active = 0;
    auto node = runtime.FirstNode(sentinel);
    while (node != sentinel) {
        const auto object = runtime.NodeObject(node);
        runtime.InvokeShutdown(object, 0x34u);
        if (object != 0u) {
            runtime.DeleteObject(object, 1u);
        }
        node = runtime.RemoveNode(state, node);
    }
    if (state.frame_state != 0u) {
        const auto camera = state.frame_state;
        runtime.CleanupCamera(camera);
        const auto result = runtime.DestroyCameraRecord(camera);
        state.frame_state = 0u;
        return result;
    }
    return sentinel;
}

}  // namespace pal4
