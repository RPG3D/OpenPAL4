#include "pal4/game_manager_reset.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameManagerResetRuntime {
public:
    void CallHandler(const std::uint32_t handler,
                     const std::uint32_t slot_offset) override {
        Check(handler == 0x6000u,
              "stop-active virtual calls target current handler");
        events.push_back(slot_offset == 0x10u ? "stop +16" : "stop +52");
        Check(slot_offset == 0x10u || slot_offset == 0x34u,
              "only observed stop-active slots are used");
    }
    std::uint32_t FirstNode(const std::uint32_t sentinel) override {
        Check(sentinel == 0x9000u, "first-node query uses +24 sentinel");
        events.push_back("first");
        return node_count == 0u ? sentinel : 0x1000u;
    }
    std::uint32_t NodeObject(const std::uint32_t node) override {
        Check(node == 0x1000u + index * 0x100u,
              "node object read follows removal result");
        events.push_back("object");
        return 0x5000u + index * 0x100u;
    }
    void InvokeShutdown(const std::uint32_t object,
                        const std::uint32_t slot_offset) override {
        Check(object == 0x5000u + index * 0x100u && slot_offset == 0x34u,
              "shutdown uses object virtual slot +52");
        events.push_back("shutdown");
    }
    void DeleteObject(const std::uint32_t object,
                      const std::uint8_t flags) override {
        Check(object == 0x5000u + index * 0x100u && flags == 1u,
              "object deleting destructor receives flag 1");
        events.push_back("delete");
    }
    std::uint32_t RemoveNode(
        pal4::ScreenEffectDispatcherState& state,
        const std::uint32_t node) override {
        Check(state.enabled == 0u && state.active == 0 &&
                  state.current_key == 0 && state.current_handler == 0u &&
                  state.tree_field == 0x8000u &&
                  node == 0x1000u + index * 0x100u,
              "tree removal uses +20 state after flags clear");
        events.push_back("remove");
        ++index;
        return index == node_count ? state.tree_sentinel
                                   : 0x1000u + index * 0x100u;
    }
    void CleanupCamera(const std::uint32_t camera) override {
        Check(camera == 0x7000u, "camera cleanup uses +16 pointer");
        events.push_back("camera cleanup");
    }
    std::uint32_t DestroyCameraRecord(const std::uint32_t camera) override {
        Check(camera == 0x7000u, "camera destroy uses same pointer");
        events.push_back("camera destroy");
        return 0xbeefu;
    }

    std::size_t node_count{2u};
    std::size_t index{};
    std::vector<std::string> events;
};

pal4::ScreenEffectDispatcherState InitialState() {
    pal4::ScreenEffectDispatcherState state{};
    state.enabled = 1u;
    state.active = 8;
    state.current_key = 12;
    state.current_handler = 0x6000u;
    state.frame_state = 0x7000u;
    state.tree_field = 0x8000u;
    state.tree_sentinel = 0x9000u;
    return state;
}

void DisabledStatePreservesIncomingRegister() {
    auto state = InitialState();
    state.enabled = 0u;
    Runtime runtime;
    Check(pal4::GameManagerReset::Reset(state, 0xabcdu, runtime) ==
              0xabcdu && runtime.events.empty() &&
              state.active == 8 && state.frame_state == 0x7000u,
          "disabled reset has no calls or writes and returns incoming EAX");
}

void AllObjectsBeforeCamera() {
    auto state = InitialState();
    Runtime runtime;
    Check(pal4::GameManagerReset::Reset(state, 0xabcdu, runtime) ==
              0xbeefu,
          "camera destroy result becomes reset result");
    Check(runtime.events == std::vector<std::string>({
              "stop +16", "stop +52", "first", "object", "shutdown", "delete", "remove",
              "object", "shutdown", "delete", "remove", "camera cleanup",
              "camera destroy"}),
          "nodes are shut down, deleted and removed before camera cleanup");
    Check(state.enabled == 0u && state.active == 0 &&
              state.frame_state == 0u && state.current_key == 0 &&
              state.current_handler == 0u,
          "reset clears only observed dispatcher fields");
}

void EmptyTreeAndAbsentCamera() {
    auto state = InitialState();
    state.frame_state = 0u;
    Runtime runtime;
    runtime.node_count = 0u;
    Check(pal4::GameManagerReset::Reset(state, 0xabcdu, runtime) ==
              0x9000u &&
              runtime.events == std::vector<std::string>({
                  "stop +16", "stop +52", "first"}),
          "empty tree and absent camera return sentinel without callbacks");
    pal4::GameManagerResetRuntimeStub stub;
    state = InitialState();
    Check(pal4::GameManagerReset::Reset(state, 0u, stub) == 0u &&
              state.enabled == 0u && state.frame_state == 0u,
          "linkable empty runtime completes reset without external libraries");
}

void StopActiveGatesAndFieldOrder() {
    auto state = InitialState();
    Runtime runtime;
    state.enabled = 0u;
    pal4::GameManagerReset::StopActive(state, runtime);
    Check(runtime.events.empty() && state.current_key == 12,
          "disabled stop does not change current key");
    state.enabled = 1u;
    for (const auto key : {0, 16}) {
        state.current_key = key;
        pal4::GameManagerReset::StopActive(state, runtime);
        Check(runtime.events.empty() && state.current_key == key &&
                  state.current_handler == 0x6000u && state.active == 8,
              "reserved keys do not deactivate handler");
    }
    state.current_key = 12;
    pal4::GameManagerReset::StopActive(state, runtime);
    Check(runtime.events == std::vector<std::string>({
              "stop +16", "stop +52"}) &&
              state.current_key == 0 && state.current_handler == 0u &&
              state.active == 0 && state.enabled == 1u,
          "active handler gets both calls before all active fields clear");
}

}  // namespace

int main() {
    DisabledStatePreservesIncomingRegister();
    AllObjectsBeforeCamera();
    EmptyTreeAndAbsentCamera();
    StopActiveGatesAndFieldOrder();
    std::cout << "4 game manager reset test cases passed\n";
}
