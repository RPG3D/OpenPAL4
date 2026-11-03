#include "pal4/screen_effect_dispatcher.h"

namespace pal4 {

void ScreenEffectDispatchRuntimeStub::PrepareSearch(
    ScreenEffectDispatcherState& /*state*/) {}
std::uint32_t ScreenEffectDispatchRuntimeStub::SearchNode(
    const ScreenEffectDispatcherState& state,
    std::int32_t /*key*/) { return state.tree_sentinel; }
std::int32_t ScreenEffectDispatchRuntimeStub::NodeKey(
    std::uint32_t /*node*/) { return 0; }
std::uint32_t ScreenEffectDispatchRuntimeStub::NodeHandler(
    std::uint32_t /*node*/) { return 0u; }
bool ScreenEffectDispatchRuntimeStub::TestHandler(
    std::uint32_t /*handler*/, std::uint32_t /*slot_offset*/) {
    return false;
}
void ScreenEffectDispatchRuntimeStub::ActivateHandler(
    std::uint32_t /*handler*/) {}

std::uint8_t ScreenEffectFrameRuntimeStub::ReadByte(
    std::uint32_t /*address*/) { return 0u; }
std::uint32_t ScreenEffectFrameRuntimeStub::ReadDword(
    std::uint32_t /*address*/) { return 0u; }
std::uint32_t ScreenEffectFrameRuntimeStub::GetObjectTransform(
    std::uint32_t /*object*/) { return 0u; }
void ScreenEffectFrameRuntimeStub::FrameSetIdentity(
    std::uint32_t /*frame*/) {}
void ScreenEffectFrameRuntimeStub::ApplyTransform(
    std::uint32_t /*frame*/, std::uint32_t /*transform*/,
    std::uint32_t /*flags*/) {}
void ScreenEffectFrameRuntimeStub::FrameUpdateObjects(
    std::uint32_t /*frame*/) {}
void ScreenEffectFrameRuntimeStub::CameraClear(
    std::uint32_t /*camera*/, std::uint32_t /*color_address*/,
    std::uint32_t /*flags*/) {}
void ScreenEffectFrameRuntimeStub::CameraBeginUpdate(
    std::uint32_t /*camera*/) {}
std::uint32_t ScreenEffectFrameRuntimeStub::CameraEndUpdate(
    std::uint32_t /*camera*/) { return 0u; }

std::uint32_t ScreenEffectSceneRuntimeStub::Render(
    std::uint32_t /*handler*/, std::uint32_t /*dispatcher*/,
    std::uint32_t /*frame_state*/) { return 0u; }

void ScreenEffectDispatcher::HandleScript(
    ScreenEffectDispatcherState& state, const std::int32_t key,
    IScreenEffectDispatchRuntime& runtime) {
    if (state.enabled == 0u) {
        return;
    }
    runtime.PrepareSearch(state);
    const auto node = runtime.SearchNode(state, key);
    if (node == state.tree_sentinel || key < runtime.NodeKey(node)) {
        return;
    }
    const auto handler = runtime.NodeHandler(node);
    if (handler == 0u) {
        return;
    }
    if (!runtime.TestHandler(handler, 4u) &&
        !runtime.TestHandler(handler, 0x30u)) {
        return;
    }
    runtime.ActivateHandler(handler);
    state.current_handler = handler;
    state.current_key = key;
    state.active = 1u;
}

void ScreenEffectDispatcher::PrepareRenderFrame(
    const ScreenEffectDispatcherState& state,
    const std::uint32_t input_record,
    IScreenEffectFrameRuntime& runtime) {
    if (state.enabled == 0u ||
        runtime.ReadByte(state.frame_state) == 0u) {
        return;
    }
    const auto object = runtime.ReadDword(input_record + 4u);
    const auto transform = runtime.GetObjectTransform(object);
    const auto camera = runtime.ReadDword(state.frame_state + 8u);
    const auto frame = runtime.ReadDword(camera + 4u);
    runtime.FrameSetIdentity(frame);
    runtime.ApplyTransform(frame, transform, 0u);
    runtime.FrameUpdateObjects(frame);
    runtime.CameraClear(camera, state.frame_state + 20u, 3u);
    runtime.CameraBeginUpdate(camera);
}

std::uint32_t ScreenEffectDispatcher::EndRenderFrame(
    const ScreenEffectDispatcherState& state,
    const std::uint32_t incoming_eax,
    IScreenEffectFrameRuntime& runtime) {
    if (state.enabled == 0u ||
        runtime.ReadByte(state.frame_state) == 0u) {
        return incoming_eax;
    }
    return runtime.CameraEndUpdate(
        runtime.ReadDword(state.frame_state + 8u));
}

std::uint32_t ScreenEffectDispatcher::RenderScene(
    const std::uint32_t object_address,
    const ScreenEffectDispatcherState& state,
    IScreenEffectSceneRuntime& runtime) {
    if (state.enabled == 0u || state.active <= 0 ||
        state.current_key == 0) {
        return object_address;
    }
    return runtime.Render(state.current_handler, object_address,
                          state.frame_state);
}

}  // namespace pal4
