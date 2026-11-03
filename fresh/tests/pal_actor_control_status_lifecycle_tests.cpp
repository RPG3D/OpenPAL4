#include "pal4/pal_actor_control_status_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlStatusLifecycleRuntime {
    std::vector<std::string> events;
    std::uint32_t frame{};
    bool needs_update{};
    pal4::PalActorControlState* expected_state{};

    void SetModeConditional(pal4::PalActorControlState& state,
                            std::uint32_t mode) override {
        Check(&state == expected_state && mode == 0u &&
                  state.words[195u] == 1u &&
                  state.words[196u] == 0x99u &&
                  state.words[197u] == 0u &&
                  state.words[198u] == 0u &&
                  state.words[199u] == 0u &&
                  state.words[200u] == 0u &&
                  state.words[201u] == 3u,
              "movement status fields precede mode change");
        events.emplace_back("mode");
        state.words[33u] = 2u;
    }
    std::uint32_t UiFrameManager() override {
        events.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x4000u && name == "portrait",
              "portrait frame lookup arguments");
        events.emplace_back("find");
        return frame;
    }
    void UpdateCharacterMovementState(std::uint32_t value) override {
        Check(value == frame, "portrait update frame address");
        events.emplace_back("update frame");
    }
    bool HandleNeedsUpdate(std::uint32_t* handle) override {
        Check(handle == expected_state->words.data() + 203u &&
                  expected_state->words[188u] == 0u &&
                  expected_state->words[195u] == 0u &&
                  expected_state->words[202u] == 0u,
              "clear writes before handle check");
        events.emplace_back("needs");
        return needs_update;
    }
    std::uint8_t UpdateHandle(std::uint32_t* handle) override {
        Check(handle == expected_state->words.data() + 203u,
              "clear update handle offset");
        events.emplace_back("update handle");
        return 0x7au;
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.expected_state = &state;
    state.words[33u] = 3u;
    state.words[195u] = 4u;
    pal4::PalActorControlStatusLifecycle::BeginMovement(
        state, 0x99u, runtime);
    Check(state.words[195u] == 4u && runtime.events.empty(),
          "active movement status does not reinitialize");

    state.words[195u] = 0u;
    state.words[197u] = 8u;
    state.words[200u] = 9u;
    runtime.frame = 0u;
    pal4::PalActorControlStatusLifecycle::BeginMovement(
        state, 0x99u, runtime);
    Check(state.words[201u] == 3u && state.words[33u] == 2u &&
              runtime.events == std::vector<std::string>{
                  "mode", "manager", "find"},
          "new movement status stores old mode and skips absent frame");

    runtime.events.clear();
    runtime.frame = 0x5000u;
    state.words[195u] = 0u;
    state.words[33u] = 3u;
    pal4::PalActorControlStatusLifecycle::BeginMovement(
        state, 0x99u, runtime);
    Check(runtime.events == std::vector<std::string>{
              "mode", "manager", "find", "update frame"},
          "portrait frame is updated after mode transition");

    runtime.events.clear();
    state.words[188u] = 1u;
    state.words[202u] = 1u;
    Check(pal4::PalActorControlStatusLifecycle::Clear(state, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"needs"} &&
              state.words[188u] == 0u && state.words[195u] == 0u &&
              state.words[202u] == 0u,
          "clear resets three flags and skips inactive handle");

    runtime.events.clear();
    runtime.needs_update = true;
    state.words[188u] = 1u;
    state.words[195u] = 1u;
    state.words[202u] = 1u;
    Check(pal4::PalActorControlStatusLifecycle::Clear(state, runtime) ==
              0x7au &&
              runtime.events == std::vector<std::string>{
                  "needs", "update handle"},
          "active handle update result is forwarded");
}
