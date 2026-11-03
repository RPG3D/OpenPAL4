#include "pal4/pal_actor_control_status_mode.h"

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

struct Runtime final : pal4::IActorControlStatusModeRuntime {
    pal4::PalActorControlState* state{};
    std::vector<std::string> calls;
    bool needs_update{};
    std::uint32_t ui_event{};
    std::uint32_t ui_queue{};
    std::uint32_t audio_mode{};
    std::uint32_t reset_object{};
    long double monster_context{};

    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui_manager"); return 0x5000u;
    }
    void ProcessUiEvent(std::uint32_t queue, std::uint32_t event,
                        const std::array<std::uint32_t, 2>& payload) override {
        Check(state->words[202u] == current_mode &&
                  payload == std::array<std::uint32_t, 2>{1u, 0u},
              "mode and payload visible before UI dispatch");
        ui_queue = queue; ui_event = event;
        calls.emplace_back("ui_event");
    }
    std::uint32_t current_mode{};
    void StartPlayerStatus(pal4::PalActorControlState& value,
                           float first, float second, float amount) override {
        Check(&value == state && first == 30.0f && second == 2.0f &&
                  amount == 7.0f, "player status args");
        calls.emplace_back("player_status");
    }
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t value) override {
        Check(animation == 0x4170u && value == 0u,
              "animation subobject offset and flag");
        calls.emplace_back("animation");
    }
    std::uint32_t MonsterManager() override {
        calls.emplace_back("monster_manager"); return 0x6000u;
    }
    std::uint32_t ProcessMonsters(std::uint32_t manager,
                                  long double context) override {
        Check(manager == 0x6000u, "monster manager address");
        monster_context = context;
        calls.emplace_back("monsters"); return 0x76543210u;
    }
    bool DialogNeedsUpdate(std::uint32_t* handle) override {
        Check(handle == state->words.data() + 203u,
              "dialog handle offset");
        calls.emplace_back("needs"); return needs_update;
    }
    void UpdateDialog(std::uint32_t* handle) override {
        Check(handle == state->words.data() + 203u,
              "dialog update handle");
        calls.emplace_back("update_dialog");
    }
    pal4::ActorControlEffectCreation CreateEffect(
        std::string_view name, std::uint32_t position,
        std::uint32_t first_flag, std::uint32_t second_flag) override {
        Check(name == "H_212" && position == 0x40acu &&
                  first_flag == 0xffffffffu && second_flag == 0u,
              "status effect args");
        calls.emplace_back("effect");
        return {{0xaabbccddu, 0x11223344u}, 0u};
    }
    void ResetTemporary(pal4::ActorControlEffectHandle& temporary) override {
        Check(state->words[203u] == temporary.object &&
                  state->words[204u] == temporary.token,
              "effect handle copied before reset");
        reset_object = temporary.object;
        calls.emplace_back("reset");
        temporary = {0xffffffffu, 0u};
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio_manager"); return 0x7000u;
    }
    std::uint32_t PlayAudio(std::uint32_t manager,
                            const std::uint32_t* mode,
                            std::string_view path,
                            std::uint32_t flag) override {
        Check(manager == 0x7000u && mode != nullptr && *mode == 1u &&
                  path == "gamedata\\PALSound\\WA30.wav" && flag == 1u,
              "status audio args");
        audio_mode = *mode;
        calls.emplace_back("audio"); return 0xcafebabeu;
    }
};

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.state = &state;
    state.words[202u] = 0x777u;
    Check(pal4::PalActorControlStatusMode::Activate(
              state, 2.5L, 1u, runtime) == 0u &&
              state.words[202u] == 0x777u && runtime.calls.empty(),
          "missing actor leaves mode and external systems untouched");

    state.words[1u] = 0x4000u;
    for (const auto mode : {0u, 4u, 0xffffffffu}) {
        runtime.calls.clear();
        Check(pal4::PalActorControlStatusMode::Activate(
                  state, 2.5L, mode, runtime) == mode - 3u &&
                  state.words[202u] == mode && runtime.calls.empty(),
              "unknown mode stores value and returns decremented EAX");
    }

    for (const auto mode : {2u, 3u}) {
        runtime.calls.clear();
        runtime.current_mode = mode;
        Check(pal4::PalActorControlStatusMode::Activate(
                  state, 2.5L, mode, runtime) == 0x76543210u &&
                  runtime.ui_event == 0x8b1320u &&
                  runtime.ui_queue == 0x5104u &&
                  runtime.monster_context == 2.5L &&
                  runtime.calls == std::vector<std::string>{
                      "ui_manager", "ui_event", "animation",
                      "monster_manager", "monsters"},
              "modes two and three share UI animation and monster path");
    }

    runtime.calls.clear();
    runtime.current_mode = 1u;
    runtime.needs_update = false;
    Check(pal4::PalActorControlStatusMode::Activate(
              state, 3.25L, 1u, runtime) == 0xcafebabeu &&
              runtime.ui_event == 0x8b1204u &&
              runtime.monster_context == 3.25L &&
              state.words[203u] == 0xaabbccddu &&
              state.words[204u] == 0x11223344u &&
              runtime.reset_object == 0xaabbccddu &&
              runtime.audio_mode == 1u &&
              runtime.calls == std::vector<std::string>{
                  "ui_manager", "ui_event", "player_status", "animation",
                  "monster_manager", "monsters", "needs", "effect", "reset",
                  "audio_manager", "audio"},
          "mode one full status effect and audio sequence");

    runtime.calls.clear();
    runtime.needs_update = true;
    Check(pal4::PalActorControlStatusMode::Activate(
              state, 3.25L, 1u, runtime) == 0xcafebabeu &&
              runtime.calls == std::vector<std::string>{
                  "ui_manager", "ui_event", "player_status", "animation",
                  "monster_manager", "monsters", "needs", "update_dialog",
                  "effect", "reset", "audio_manager", "audio"},
          "dialog update occurs only when requested");
}
