#include "pal4/screen_effect_dispatcher.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class DispatchRuntime final : public pal4::IScreenEffectDispatchRuntime {
public:
    void PrepareSearch(
        pal4::ScreenEffectDispatcherState& state) override {
        Check(state.tree_field == 0x5555u,
              "search preparation receives dispatcher tree at +20");
        events.push_back("prepare");
    }
    std::uint32_t SearchNode(
        const pal4::ScreenEffectDispatcherState& state,
        std::int32_t key) override {
        Check(state.tree_sentinel == 0x9999u && key == 42,
              "tree search receives key and sentinel state");
        events.push_back("search");
        return search_result;
    }
    std::int32_t NodeKey(std::uint32_t node) override {
        Check(node == 0x2000u, "node-key read uses search result");
        events.push_back("node key");
        return node_key;
    }
    std::uint32_t NodeHandler(std::uint32_t node) override {
        Check(node == 0x2000u, "handler read uses node +16");
        events.push_back("node handler");
        return handler;
    }
    bool TestHandler(std::uint32_t object,
                     std::uint32_t slot_offset) override {
        Check(object == 0x3000u, "virtual test receives handler object");
        events.push_back(slot_offset == 4u ? "test +4" : "test +48");
        Check(slot_offset == 4u || slot_offset == 0x30u,
              "only observed gate slots are used");
        return slot_offset == 4u ? first_ready : fallback_ready;
    }
    void ActivateHandler(std::uint32_t object) override {
        Check(object == 0x3000u, "activation receives handler");
        events.push_back("activate +12");
    }

    std::uint32_t search_result{0x2000u};
    std::int32_t node_key{42};
    std::uint32_t handler{0x3000u};
    bool first_ready{true};
    bool fallback_ready{};
    std::vector<std::string> events;
};

class FrameRuntime final : public pal4::IScreenEffectFrameRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x4000u, "inner frame flag at +0");
        events.push_back("frame flag");
        return frame_enabled;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == 0x5004u) {
            events.push_back("object");
            return 0x6000u;
        }
        if (address == 0x4008u) {
            events.push_back("camera");
            return 0x7000u;
        }
        Check(address == 0x7004u, "camera frame pointer at +4");
        events.push_back("frame");
        return 0x8000u;
    }
    std::uint32_t GetObjectTransform(std::uint32_t object) override {
        Check(object == 0x6000u, "transform lookup object");
        events.push_back("transform");
        return 0x9000u;
    }
    void FrameSetIdentity(std::uint32_t frame) override {
        Check(frame == 0x8000u, "identity frame");
        events.push_back("identity");
    }
    void ApplyTransform(std::uint32_t frame,
                        std::uint32_t transform,
                        std::uint32_t flags) override {
        Check(frame == 0x8000u && transform == 0x9000u && flags == 0u,
              "transform apply arguments");
        events.push_back("apply transform");
    }
    void FrameUpdateObjects(std::uint32_t frame) override {
        Check(frame == 0x8000u, "frame update object");
        events.push_back("update objects");
    }
    void CameraClear(std::uint32_t camera,
                     std::uint32_t color_address,
                     std::uint32_t flags) override {
        Check(camera == 0x7000u && color_address == 0x4014u &&
                  flags == 3u,
              "camera clear uses nested color and flag 3");
        events.push_back("camera clear");
    }
    void CameraBeginUpdate(std::uint32_t camera) override {
        Check(camera == 0x7000u, "camera begin target");
        events.push_back("camera begin");
    }
    std::uint32_t CameraEndUpdate(std::uint32_t camera) override {
        Check(camera == 0x7000u, "camera end target");
        events.push_back("camera end");
        return 0x7777u;
    }

    std::uint8_t frame_enabled{1u};
    std::vector<std::string> events;
};

class SceneRuntime final : public pal4::IScreenEffectSceneRuntime {
public:
    std::uint32_t Render(std::uint32_t handler,
                         std::uint32_t dispatcher,
                         std::uint32_t frame_state) override {
        Check(handler == 0x3000u && dispatcher == 0x1000u &&
                  frame_state == 0x4000u,
              "scene callback receives handler, this and frame state");
        ++calls;
        return 0x8888u;
    }
    int calls{};
};

pal4::ScreenEffectDispatcherState State() {
    return {1u, {0u, 0u, 0u}, 0u, -1, 0u,
            0x4000u, 0x5555u, 0x9999u};
}

void SearchGatesPreserveState() {
    auto state = State();
    DispatchRuntime runtime;
    state.enabled = 0u;
    pal4::ScreenEffectDispatcher::HandleScript(state, 42, runtime);
    Check(runtime.events.empty() && state.active == 0u,
          "disabled dispatcher skips search");
    state.enabled = 1u;
    runtime.search_result = 0x9999u;
    pal4::ScreenEffectDispatcher::HandleScript(state, 42, runtime);
    Check(runtime.events == std::vector<std::string>({
              "prepare", "search"}) && state.active == 0u,
          "sentinel search result avoids node dereference");
    runtime.search_result = 0x2000u;
    runtime.node_key = 43;
    runtime.events.clear();
    pal4::ScreenEffectDispatcher::HandleScript(state, 42, runtime);
    Check(runtime.events == std::vector<std::string>({
              "prepare", "search", "node key"}) &&
              state.current_handler == 0u,
          "key before candidate falls back to sentinel");
    runtime.node_key = 42;
    runtime.handler = 0u;
    runtime.events.clear();
    pal4::ScreenEffectDispatcher::HandleScript(state, 42, runtime);
    Check(runtime.events == std::vector<std::string>({
              "prepare", "search", "node key", "node handler"}) &&
              state.active == 0u,
          "empty node handler does not change state");
}

void HandlerVirtualGatePaths() {
    auto state = State();
    DispatchRuntime runtime;
    runtime.first_ready = false;
    pal4::ScreenEffectDispatcher::HandleScript(state, 42, runtime);
    Check(runtime.events == std::vector<std::string>({
              "prepare", "search", "node key", "node handler",
              "test +4", "test +48"}) && state.active == 0u,
          "both false gates return before activation");
    runtime.fallback_ready = true;
    runtime.events.clear();
    pal4::ScreenEffectDispatcher::HandleScript(state, 42, runtime);
    Check(runtime.events.back() == "activate +12" &&
              state.active == 1u && state.current_key == 42 &&
              state.current_handler == 0x3000u,
          "fallback gate activates and records script handler");
    state = State();
    runtime.first_ready = true;
    runtime.fallback_ready = false;
    runtime.events.clear();
    pal4::ScreenEffectDispatcher::HandleScript(state, 42, runtime);
    Check(runtime.events == std::vector<std::string>({
              "prepare", "search", "node key", "node handler",
              "test +4", "activate +12"}) && state.active == 1u,
          "first gate skips fallback virtual call");
    pal4::ScreenEffectDispatchRuntimeStub stub;
    state = State();
    pal4::ScreenEffectDispatcher::HandleScript(state, 42, stub);
    Check(state.active == 0u, "linkable dispatch stub ends at sentinel");
}

void FrameRenderGatesAndOrder() {
    auto state = State();
    FrameRuntime runtime;
    state.enabled = 0u;
    pal4::ScreenEffectDispatcher::PrepareRenderFrame(
        state, 0x5000u, runtime);
    Check(runtime.events.empty(), "outer frame flag blocks memory reads");
    state.enabled = 1u;
    runtime.frame_enabled = 0u;
    pal4::ScreenEffectDispatcher::PrepareRenderFrame(
        state, 0x5000u, runtime);
    Check(runtime.events == std::vector<std::string>{"frame flag"},
          "inner frame flag blocks RenderWare calls");
    runtime.frame_enabled = 1u;
    runtime.events.clear();
    pal4::ScreenEffectDispatcher::PrepareRenderFrame(
        state, 0x5000u, runtime);
    Check(runtime.events == std::vector<std::string>({
              "frame flag", "object", "transform", "camera", "frame",
              "identity", "apply transform", "update objects",
              "camera clear", "camera begin"}),
          "frame preparation follows observed RenderWare call order");
    pal4::ScreenEffectFrameRuntimeStub stub;
    pal4::ScreenEffectDispatcher::PrepareRenderFrame(
        state, 0x5000u, stub);
}

void EndFramePreservesUndefinedReturnOnGuardExit() {
    auto state = State();
    FrameRuntime runtime;
    state.enabled = 0u;
    Check(pal4::ScreenEffectDispatcher::EndRenderFrame(
              state, 0xaaaau, runtime) == 0xaaaau &&
              runtime.events.empty(),
          "outer guard preserves incoming EAX without memory read");
    state.enabled = 1u;
    runtime.frame_enabled = 0u;
    Check(pal4::ScreenEffectDispatcher::EndRenderFrame(
              state, 0xaaaau, runtime) == 0xaaaau &&
              runtime.events == std::vector<std::string>{"frame flag"},
          "inner guard preserves incoming EAX");
    runtime.frame_enabled = 1u;
    runtime.events.clear();
    Check(pal4::ScreenEffectDispatcher::EndRenderFrame(
              state, 0xaaaau, runtime) == 0x7777u &&
              runtime.events == std::vector<std::string>({
                  "frame flag", "camera", "camera end"}),
          "active frame returns RenderWare camera-end result");
}

void SceneRenderRequiresThreeFields() {
    auto state = State();
    SceneRuntime runtime;
    state.current_handler = 0x3000u;
    state.active = 1;
    state.current_key = 42;
    state.enabled = 0u;
    Check(pal4::ScreenEffectDispatcher::RenderScene(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.calls == 0,
          "disabled scene returns this");
    state.enabled = 1u;
    state.active = -1;
    Check(pal4::ScreenEffectDispatcher::RenderScene(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.calls == 0,
          "nonpositive active counter blocks render");
    state.active = 1;
    state.current_key = 0;
    Check(pal4::ScreenEffectDispatcher::RenderScene(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.calls == 0,
          "zero current key blocks render");
    state.current_key = 42;
    Check(pal4::ScreenEffectDispatcher::RenderScene(
              0x1000u, state, runtime) == 0x8888u &&
              runtime.calls == 1,
          "scene forwards virtual +0x14 result");
    pal4::ScreenEffectSceneRuntimeStub stub;
    Check(pal4::ScreenEffectDispatcher::RenderScene(
              0x1000u, state, stub) == 0u,
          "linkable scene stub replaces renderer callback");
}

}  // namespace

int main() {
    SearchGatesPreserveState();
    HandlerVirtualGatePaths();
    FrameRenderGatesAndOrder();
    EndFramePreservesUndefinedReturnOnGuardExit();
    SceneRenderRequiresThreeFields();
    std::cout << "5 screen effect dispatcher test cases passed\n";
}
