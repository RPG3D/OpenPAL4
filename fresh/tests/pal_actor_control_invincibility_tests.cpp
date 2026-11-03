#include "pal4/pal_actor_control_invincibility.h"

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

struct Runtime : pal4::IActorControlInvincibilityRuntime {
    std::vector<std::string> events;
    std::uint32_t* expected_handle{};
    bool needs_update{};

    bool NeedsUpdate(std::uint32_t* handle) override {
        Check(handle == expected_handle, "old effect handle offset");
        events.emplace_back("needs");
        return needs_update;
    }
    void Update(std::uint32_t* handle) override {
        Check(handle == expected_handle, "update old effect handle");
        events.emplace_back("update");
    }
    pal4::ActorControlEffectCreation CreateEffect(
        std::string_view name, std::uint32_t parameter_address,
        std::uint32_t first_flag,
        std::uint32_t second_flag) override {
        Check(name == "invincible" && parameter_address == 0x12e0u &&
                  first_flag == 0xffffffffu && second_flag == 0u,
              "invincible effect creation arguments");
        events.emplace_back("create");
        return {{0x40u, 0x50u}, 0x6000u};
    }
    void ResetTemporary(pal4::ActorControlEffectHandle& handle) override {
        Check(expected_handle[0] == 0x40u &&
                  expected_handle[1] == 0x50u &&
                  handle.object == 0x40u && handle.token == 0x50u,
              "effect handle copied before temporary reset");
        IActorControlInvincibilityRuntime::ResetTemporary(handle);
        Check(handle.object == 0xffffffffu && handle.token == 0u,
              "temporary reset values");
        events.emplace_back("reset");
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.expected_handle = state.words.data() + 186u;
    state.words[184u] = 7u;
    Check(pal4::PalActorControlInvincibility::Enable(
              state, 0x99u, runtime) == 0u &&
              state.words[184u] == 7u && runtime.events.empty(),
          "null actor exits without touching state or runtime");

    state.words[1u] = 0x1234u;
    Check(pal4::PalActorControlInvincibility::Enable(
              state, 0x99u, runtime) == 0x6000u &&
              state.words[184u] == 1u && state.words[185u] == 0x99u &&
              state.words[186u] == 0x40u && state.words[187u] == 0x50u &&
              runtime.events == std::vector<std::string>{
                  "needs", "create", "reset"},
          "enable stores state and new effect in observed order");

    runtime.events.clear();
    runtime.needs_update = true;
    Check(pal4::PalActorControlInvincibility::Enable(
              state, 0x88u, runtime) == 0x6000u &&
              state.words[185u] == 0x88u &&
              runtime.events == std::vector<std::string>{
                  "needs", "update", "create", "reset"},
          "pending old effect is updated before replacement");
}
