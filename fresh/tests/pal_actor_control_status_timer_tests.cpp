#include "pal4/pal_actor_control_status_lifecycle.h"

#include <bit>
#include <cmath>
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

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime : pal4::IActorControlStatusLifecycleRuntime {
    pal4::PalActorControlState* state{};
    std::vector<std::string> events;
    std::vector<int> needs_results;
    std::uint32_t frame{};
    std::uint32_t position_address{};
    std::uint32_t delta_bits{};

    void HandleDamageOrStatus(pal4::PalActorControlState& current,
                              float value) override {
        Check(&current == state && current.words[188u] != 0u &&
                  current.words[192u] == 0u && Bits(value) == Bits(0.75f),
              "damage callback follows second-timer reset while active");
        events.emplace_back("damage");
    }
    void SetModeConditional(pal4::PalActorControlState& current,
                            std::uint32_t mode) override {
        Check(&current == state && current.words[195u] == 0u &&
                  mode == 3u,
              "saved mode restored after movement flag clears");
        events.emplace_back("mode");
    }
    std::uint32_t UiFrameManager() override {
        events.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x4000u && name == "portrait",
              "timer portrait frame lookup");
        events.emplace_back("find");
        return frame;
    }
    void UpdateCharacterMovementState(std::uint32_t value) override {
        Check(value == frame, "timer frame update address");
        events.emplace_back("frame");
    }
    bool HandleNeedsUpdate(std::uint32_t* handle) override {
        Check(handle == state->words.data() + 203u,
              "timer handle offset");
        events.emplace_back("needs");
        Check(!needs_results.empty(), "missing handle-check result");
        const bool result = needs_results.front() != 0;
        needs_results.erase(needs_results.begin());
        return result;
    }
    std::uint8_t UpdateHandle(std::uint32_t* handle) override {
        Check(handle == state->words.data() + 203u,
              "expired movement handle update");
        events.emplace_back("handle");
        return 0x23u;
    }
    std::uint8_t UpdateEffectProperties(
        std::uint32_t* handle, std::uint32_t actor_position_address,
        std::uint32_t value_bits) override {
        Check(handle == state->words.data() + 203u,
              "effect property handle");
        position_address = actor_position_address;
        delta_bits = value_bits;
        events.emplace_back("effect");
        return 0x7au;
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.state = &state;
    runtime.needs_results = {0};
    Check(pal4::PalActorControlStatusLifecycle::Tick(
              state, 0.25f, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"needs"},
          "inactive timers still check effect handle once");

    runtime.events.clear();
    runtime.needs_results = {0};
    state.words[188u] = 1u;
    state.words[189u] = Bits(10.0f);
    state.words[190u] = 0u;
    state.words[191u] = Bits(0.2f);
    state.words[192u] = 0u;
    state.words[193u] = Bits(0.75f);
    Check(pal4::PalActorControlStatusLifecycle::Tick(
              state, 0.25f, runtime) == 0u &&
              state.words[188u] == 1u &&
              state.words[190u] == Bits(0.25f) &&
              state.words[192u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "damage", "needs"},
          "second status timer retriggers while first remains active");

    runtime.events.clear();
    runtime.needs_results = {0};
    state.words[189u] = Bits(0.5f);
    Check(pal4::PalActorControlStatusLifecycle::Tick(
              state, 0.25f, runtime) == 0u &&
              state.words[188u] == 0u && state.words[192u] == 0u &&
              runtime.events == std::vector<std::string>{"needs"},
          "first status expiry suppresses simultaneous damage callback");

    runtime.events.clear();
    runtime.needs_results = {0};
    state.words[188u] = 1u;
    state.words[190u] = 0u;
    state.words[192u] = 0u;
    Check(pal4::PalActorControlStatusLifecycle::Tick(
              state, NAN, runtime) == 0u &&
              state.words[188u] == 1u &&
              std::isnan(std::bit_cast<float>(state.words[190u])) &&
              std::isnan(std::bit_cast<float>(state.words[192u])) &&
              runtime.events == std::vector<std::string>{"needs"},
          "unordered status comparisons do not expire timers");

    runtime.events.clear();
    runtime.needs_results = {0};
    state.words[188u] = 0u;
    state.words[195u] = 1u;
    state.words[196u] = Bits(0.5f);
    state.words[197u] = 0u;
    state.words[201u] = 3u;
    Check(pal4::PalActorControlStatusLifecycle::Tick(
              state, 0.1f, runtime) == 0u &&
              state.words[195u] == 1u &&
              state.words[197u] == Bits(0.1f) &&
              runtime.events == std::vector<std::string>{"needs"},
          "movement timer below threshold keeps mode and flag");

    runtime.events.clear();
    runtime.needs_results = {0, 0};
    Check(pal4::PalActorControlStatusLifecycle::Tick(
              state, 0.5f, runtime) == 0u &&
              state.words[195u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "mode", "manager", "find", "needs", "needs"},
          "movement expiry checks handle both before and after UI work");

    runtime.events.clear();
    runtime.frame = 0x5000u;
    runtime.needs_results = {1, 1};
    state.words[195u] = 1u;
    state.words[197u] = 0u;
    state.words[1u] = 0u;
    Check(pal4::PalActorControlStatusLifecycle::Tick(
              state, 0.75f, runtime) == 0x7au &&
              runtime.position_address == 0xacu &&
              runtime.delta_bits == (Bits(0.75f) & 0xffffff00u) &&
              runtime.events == std::vector<std::string>{
                  "mode", "manager", "find", "frame", "needs", "handle",
                  "needs", "effect"},
          "final effect update clears delta low byte and does not gate actor");
}
