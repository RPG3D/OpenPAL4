#include "pal4/pal_actor_control_status_mode.h"

#include <algorithm>
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
    std::vector<bool> needs_results;
    std::uint32_t frame{0x6000u};
    std::uint32_t player{0x7000u};
    bool visible{};
    bool in_range{};
    std::uint32_t effect_flag{};
    std::string effect_name;
    std::string audio_path;

    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui_manager"); return 0x5000u;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x5000u &&
                  name == (state->words[202u] == 1u ?
                      "ClickGameWindow" : "RepeatGameWindow"),
              "mode-specific frame name");
        calls.emplace_back("find_frame"); return frame;
    }
    bool IsFrameVisible(std::uint32_t value) override {
        Check(value == frame, "visible frame address");
        calls.emplace_back("visible"); return visible;
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("player_manager"); return 0x7100u;
    }
    std::uint32_t FindPlayer(std::uint32_t manager,
                             std::uint32_t index) override {
        Check(manager == 0x7100u && index == 2u,
              "mode-one player lookup");
        calls.emplace_back("find_player"); return player;
    }
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t value) override {
        Check(animation == 0x8170u && value == 0x3f800000u,
              "animation flag restored to float one");
        calls.emplace_back("animation");
    }
    std::uint32_t MonsterManager() override {
        calls.emplace_back("monster_manager"); return 0x9000u;
    }
    void ClearMonsterFlags(std::uint32_t manager) override {
        Check(manager == 0x9000u, "monster manager address");
        calls.emplace_back("clear_monsters");
    }
    void ClearStatusHandles(pal4::PalActorControlState& value) override {
        Check(&value == state, "status cleanup state");
        calls.emplace_back("clear_status");
    }
    bool DialogNeedsUpdate(std::uint32_t* handle) override {
        Check(handle == state->words.data() + 203u,
              "dialog handle address");
        Check(!needs_results.empty(), "missing dialog check result");
        const bool result = needs_results.front();
        needs_results.erase(needs_results.begin());
        calls.emplace_back("needs"); return result;
    }
    void UpdateDialog(std::uint32_t* handle) override {
        Check(handle == state->words.data() + 203u,
              "dialog update address");
        calls.emplace_back("update_dialog");
    }
    bool PositionInRange(std::uint32_t value) override {
        Check(value == frame && state->words[202u] == 0u,
              "mode reset before range check");
        calls.emplace_back("range"); return in_range;
    }
    void BeginMovementStatus(pal4::PalActorControlState& value,
                             float duration) override {
        Check(&value == state && duration == 20.0f,
              "mode-three movement duration");
        calls.emplace_back("movement");
    }
    void ApplyStatusIfPlayerExists(pal4::PalActorControlState& value,
                                   float amount) override {
        Check(&value == state && amount == 25.0f,
              "mode-two status amount");
        calls.emplace_back("apply_status");
    }
    pal4::ActorControlEffectCreation CreateEffect(
        std::string_view name, std::uint32_t position,
        std::uint32_t first_flag, std::uint32_t second_flag) override {
        Check(position == 0x80acu && second_flag == 0u,
              "effect position and final flag");
        effect_name = name;
        effect_flag = first_flag;
        calls.emplace_back("effect");
        return {{0xaabbccddu, 0x11223344u}, 0u};
    }
    void ResetTemporary(pal4::ActorControlEffectHandle& temporary) override {
        Check(state->words[203u] == temporary.object &&
                  state->words[204u] == temporary.token,
              "effect copied before temporary reset");
        calls.emplace_back("reset_effect");
        temporary = {0xffffffffu, 0u};
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio_manager"); return 0xa000u;
    }
    void PlayAudioScratch(std::uint32_t manager,
                          std::string_view path,
                          std::uint32_t flag) override {
        Check(manager == 0xa000u && flag == 1u,
              "audio manager and flag");
        audio_path = path;
        calls.emplace_back("audio");
    }
};

void Reset(Runtime& runtime) {
    runtime.calls.clear();
    runtime.needs_results.clear();
    runtime.frame = 0x6000u;
    runtime.player = 0x7000u;
    runtime.visible = false;
    runtime.in_range = false;
    runtime.effect_name.clear();
    runtime.audio_path.clear();
}

}  // namespace

int main() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.state = &state;
    state.words[1u] = 0x8000u;
    state.words[19u] = 2u;

    for (const auto mode : {0u, 4u, 0xffffffffu}) {
        Reset(runtime);
        state.words[202u] = mode;
        Check(pal4::PalActorControlStatusMode::Poll(state, runtime) == 0u &&
                  state.words[202u] == mode && runtime.calls.empty(),
              "unsupported mode does nothing");
    }

    for (const auto mode : {1u, 2u, 3u}) {
        Reset(runtime);
        state.words[202u] = mode;
        runtime.frame = 0u;
        Check(pal4::PalActorControlStatusMode::Poll(state, runtime) == 0u &&
                  state.words[202u] == mode && runtime.calls ==
                      std::vector<std::string>{"ui_manager", "find_frame"},
              "missing frame preserves mode");
    }

    Reset(runtime);
    state.words[202u] = 1u;
    runtime.player = 0u;
    Check(pal4::PalActorControlStatusMode::Poll(state, runtime) == 0u &&
              state.words[202u] == 1u &&
              runtime.calls == std::vector<std::string>{
                  "ui_manager", "find_frame", "player_manager", "find_player",
                  "animation", "monster_manager", "clear_monsters",
                  "clear_status"},
          "missing mode-one player clears state before visibility check");

    for (const auto mode : {1u, 2u, 3u}) {
        Reset(runtime);
        state.words[202u] = mode;
        runtime.visible = true;
        Check(pal4::PalActorControlStatusMode::Poll(state, runtime) == 1u &&
                  state.words[202u] == mode &&
                  runtime.calls.back() == "visible" &&
                  runtime.calls.size() == (mode == 1u ? 5u : 3u),
              "visible frame returns one without cleanup");
    }

    Reset(runtime);
    state.words[202u] = 1u;
    Check(pal4::PalActorControlStatusMode::Poll(state, runtime) == 0u &&
              state.words[202u] == 1u &&
              runtime.audio_path == "gamedata\\PALSound\\WA31.wav" &&
              runtime.calls == std::vector<std::string>{
                  "ui_manager", "find_frame", "player_manager", "find_player",
                  "visible", "animation", "monster_manager",
                  "clear_monsters", "clear_status", "audio_manager", "audio"},
          "mode-one completion preserves mode and plays WA31");

    for (const auto mode : {2u, 3u}) {
        Reset(runtime);
        state.words[202u] = mode;
        runtime.in_range = true;
        runtime.needs_results = {true};
        Check(pal4::PalActorControlStatusMode::Poll(state, runtime) == 0u &&
                  state.words[202u] == 0u &&
                  runtime.audio_path == "gamedata\\PALSound\\WA32.wav" &&
                  runtime.calls == std::vector<std::string>{
                      "ui_manager", "find_frame", "visible", "animation",
                      "monster_manager", "clear_monsters", "needs",
                      "update_dialog", "range", "audio_manager", "audio"},
              "mode-two and three in-range completion shares WA32 path");
    }

    Reset(runtime);
    state.words[202u] = 2u;
    runtime.needs_results = {false, true};
    Check(pal4::PalActorControlStatusMode::Poll(state, runtime) == 0u &&
              state.words[202u] == 0u &&
              runtime.effect_name == "H_188" && runtime.effect_flag == 1u &&
              runtime.audio_path.empty() &&
              runtime.calls == std::vector<std::string>{
                  "ui_manager", "find_frame", "visible", "animation",
                  "monster_manager", "clear_monsters", "needs", "range",
                  "apply_status", "needs", "update_dialog", "effect",
                  "reset_effect"},
          "mode-two out-of-range applies status without audio");

    Reset(runtime);
    state.words[202u] = 3u;
    runtime.needs_results = {true, false};
    Check(pal4::PalActorControlStatusMode::Poll(state, runtime) == 0u &&
              state.words[202u] == 0u &&
              runtime.effect_name == "H_189" &&
              runtime.effect_flag == 0xffffffffu &&
              runtime.audio_path == "gamedata\\PALSound\\WA29.wav" &&
              runtime.calls == std::vector<std::string>{
                  "ui_manager", "find_frame", "visible", "animation",
                  "monster_manager", "clear_monsters", "needs",
                  "update_dialog", "range", "movement", "needs", "effect",
                  "reset_effect", "audio_manager", "audio"},
          "mode-three out-of-range starts movement and plays WA29");
}
