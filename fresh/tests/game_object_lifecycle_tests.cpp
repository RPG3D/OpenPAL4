#include "pal4/game_object_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::uint32_t kAddress = 0x10000u;

struct Runtime final : pal4::GameObjectLifecycleRuntimeStub {
    std::vector<std::string> calls;

    void CleanupEventsAndScripts(pal4::GameObjectState& state,
                                 std::uint32_t address,
                                 std::uint32_t mode) override {
        Check(address == kAddress && mode == 1u &&
              state.words[0x654u / 4u] == 0xabu,
              "cleanup arguments and pre-clear state");
        calls.emplace_back("cleanup");
    }
    std::uint32_t Reset(pal4::GameObjectState& state,
                        std::uint32_t address) override {
        Check(address == kAddress && state.words[0x654u / 4u] == 0u,
              "reset runs after status clear");
        calls.emplace_back("reset");
        return 0x1234u;
    }
    std::uint32_t UpdateState(pal4::GameObjectState&,
                              std::uint32_t address,
                              std::uint32_t argument) override {
        Check(address == kAddress && argument == 0x55u,
              "state-update arguments");
        calls.emplace_back("update-state");
        return 0x5678u;
    }
    void HandleEventProcessing(std::uint32_t processor,
                               float delta_time, std::uint32_t script,
                               std::uint32_t tree,
                               std::uint32_t context) override {
        Check(processor == kAddress + 0x658u && delta_time == 1.25f &&
              script == kAddress + 0x688u &&
              tree == kAddress + 0x698u &&
              context == kAddress + 0x6acu,
              "script-processing argument order and offsets");
        calls.emplace_back("handle-event");
    }
    std::uint32_t UpdatePosition(pal4::GameObjectState&,
                                 std::uint32_t address,
                                 float delta_time,
                                 std::uint32_t argument) override {
        Check(address == kAddress && delta_time == 1.25f &&
              argument == 0x66u, "position-update arguments");
        calls.emplace_back("update-position");
        return 0x9abcu;
    }
};

}  // namespace

int main() {
    pal4::GameObjectState state{};
    state.words.fill(0xdeadbeefu);
    Runtime runtime;
    state.words[0x654u / 4u] = 0xabu;
    Check(pal4::GameObjectLifecycle::CleanupVirtual(
              state, kAddress, runtime) == 0x1234u &&
          state.words[0x654u / 4u] == 0u &&
          state.words[0x650u / 4u] == 0xdeadbeefu &&
          state.words[0x658u / 4u] == 0xdeadbeefu &&
          runtime.calls == std::vector<std::string>{"cleanup", "reset"},
          "virtual cleanup order, exact write and tail return");

    runtime.calls.clear();
    state.words[0x1f0u / 4u] = 0u;
    Check(pal4::GameObjectLifecycle::ConditionalProcess(
              state, kAddress, 0x55u, runtime) == 0u &&
          pal4::GameObjectLifecycle::HandleEventAndProcess(
              state, kAddress, 1.25f, 0x66u, runtime) == 0u &&
          runtime.calls.empty(), "inactive object skips both process paths");

    state.words[0x1f0u / 4u] = 3u;
    Check(pal4::GameObjectLifecycle::ConditionalProcess(
              state, kAddress, 0x55u, runtime) == 0x5678u &&
          pal4::GameObjectLifecycle::HandleEventAndProcess(
              state, kAddress, 1.25f, 0x66u, runtime) == 0x9abcu &&
          runtime.calls == std::vector<std::string>{
              "update-state", "handle-event", "update-position"},
          "active object forwards state, events and position in order");

    pal4::GameObjectState composed{};
    composed.words[0x654u / 4u] = 0x9000u;
    composed.words[0x6d8u / 4u] = 0xb000u;
    pal4::GameObjectLifecycleRuntimeStub default_runtime;
    Check(pal4::GameObjectLifecycle::CleanupVirtual(
              composed, kAddress, default_runtime) == 0u &&
          composed.words[0x654u / 4u] == 0u &&
          composed.words[0x6d8u / 4u] == 0u,
          "default cleanup composes the reconstructed event cleanup");
}
