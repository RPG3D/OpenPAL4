#include "pal4/minimap_scene_ui_input.h"

#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMinimapSceneUiInputRuntime {
    std::vector<std::string> calls;
    std::uint32_t event_word = 33u;
    std::int32_t control_word = 0;
    bool written = false;
    int control_calls = 0;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> player_states;
    std::vector<std::uint32_t> players{0x9000u, 0xa000u, 0u, 0u};
    std::int32_t active_count = 2;
    bool visible = true;
    bool throw_in_sort = false;
    std::uint32_t dispatched_kind = 0u;
    std::uint32_t refreshed_player = 0u;
    std::uint32_t refreshed_action = 0u;
    std::vector<std::uint32_t> sorted_players;

    std::uint32_t ReadEventWord(std::uint32_t address) override {
        Check(address == 0x6010u, "event code at +0x10");
        calls.emplace_back("event");
        return event_word;
    }
    std::uint32_t ActorControl() override {
        ++control_calls;
        calls.emplace_back("control");
        return control_calls == 1 ? 0x7000u : 0x8000u;
    }
    std::int32_t GetControlWord(std::uint32_t control) override {
        Check(control == 0x8000u, "second control supplies current word");
        calls.emplace_back("read-word");
        return control_word;
    }
    void SetControlWord(std::uint32_t control,
                        bool enabled) override {
        Check(control == 0x7000u, "first control receives new word");
        written = enabled;
        calls.emplace_back("write-word");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        return words[address];
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x7000u;
    }
    void SwitchSystemState(std::uint32_t owner) override {
        Check(owner == 0x5000u, "system state owner");
        calls.emplace_back("switch");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("frame");
        return 0x8000u;
    }
    void DispatchEvent(std::uint32_t dispatcher,
                       std::uint32_t key_cell,
                       pal4::MinimapSceneUiEventData data) override {
        Check(dispatcher == 0x8104u &&
                  key_cell == pal4::MinimapSceneUiInput::kSystemEventKeyCell &&
                  data.value == 0u,
              "system event dispatcher and payload");
        dispatched_kind = data.kind;
        calls.emplace_back("dispatch");
    }
    void AdvanceRunMode(std::uint32_t owner,
                        std::uint32_t adapter) override {
        Check(owner == 0x5000u && adapter == 0x5028u,
              "run mode owner and input adapter");
        calls.emplace_back("run");
    }
    void ToggleState(std::uint32_t owner) override {
        Check(owner == 0x5000u, "toggle owner");
        calls.emplace_back("toggle");
    }
    bool WindowVisible(std::uint32_t window) override {
        Check(window == 0x6400u || window == 0x6800u,
              "window selected from owner layout");
        calls.emplace_back(window == 0x6400u ? "window64" : "window68");
        return visible;
    }
    void ActivateSkill(std::uint32_t owner) override {
        Check(owner == 0x5000u, "skill owner");
        calls.emplace_back("skill");
    }
    void UpdateSkillState(std::uint32_t owner) override {
        Check(owner == 0x5000u, "skill state owner");
        calls.emplace_back("skill-state");
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0xb000u;
    }
    std::int32_t ActivePlayerCount(std::uint32_t manager) override {
        Check(manager == 0xb000u, "active count manager");
        calls.emplace_back("count");
        return active_count;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0xb000u && index < 4u, "player index");
        calls.emplace_back("player" + std::to_string(index));
        return players[index];
    }
    void SortPlayers(std::span<std::uint32_t> values) override {
        calls.emplace_back("sort");
        if (throw_in_sort) {
            throw std::runtime_error("sort failed");
        }
        std::sort(values.begin(), values.end());
        sorted_players.assign(values.begin(), values.end());
    }
    std::uint32_t PlayerState(std::uint32_t player) override {
        calls.emplace_back("get-state:" + std::to_string(player));
        return player_states[player];
    }
    void SetPlayerState(std::uint32_t player,
                        std::uint32_t value) override {
        calls.emplace_back("set-state:" + std::to_string(player));
        player_states[player] = value;
    }
    void UpdatePlayerStates(std::uint32_t manager,
                            std::uint32_t action) override {
        Check(manager == 0xb000u, "player update manager");
        refreshed_action = action;
        calls.emplace_back("update-states");
    }
    void UpdatePlayerAndGameStates(std::uint32_t owner) override {
        Check(owner == 0x5000u, "UI refresh owner");
        calls.emplace_back("refresh-ui");
    }
};

}  // namespace

int main() {
    Runtime other;
    other.event_word = 32u;
    Check(pal4::MinimapSceneUiInput::HandleSpecialKey(
              0x6000u, other) == 0u &&
              other.calls == std::vector<std::string>{"event"} &&
              other.control_calls == 0,
          "other key returns zero without actor control access");

    for (const auto word : {0, 1, -1}) {
        Runtime matched;
        matched.control_word = word;
        Check(pal4::MinimapSceneUiInput::HandleSpecialKey(
                  0x6000u, matched) == 1u &&
                  matched.written == (word == 0) &&
                  matched.control_calls == 2 &&
                  matched.calls == std::vector<std::string>{
                                       "event", "control", "control",
                                       "read-word", "write-word"},
              "special key flips control word after two singleton accesses");
    }

    pal4::MinimapSceneUiState state;
    state.WriteWord(0x64u, 0x6400u);
    state.WriteWord(0x68u, 0x6800u);
    const auto handle = [&](Runtime& runtime) {
        return pal4::MinimapSceneUiInput::HandlePlayerInput(
            state, 0x5000u, 0x6000u, runtime);
    };

    for (const auto code : {0u, 14u, 16u, 58u, 66u}) {
        Runtime runtime;
        runtime.event_word = code;
        Check(handle(runtime) == 0u &&
                  runtime.calls == std::vector<std::string>{"event"},
              "unknown event returns zero without side effects");
    }
    for (std::uint32_t code = 59u; code <= 65u; ++code) {
        Runtime runtime;
        runtime.event_word = code;
        Check(handle(runtime) == 1u &&
                  runtime.dispatched_kind == code - 58u &&
                  runtime.calls == std::vector<std::string>{
                                       "event", "switch", "frame", "dispatch"},
              "numbered event switches and dispatches in order");
    }
    {
        Runtime runtime;
        runtime.event_word = 1u;
        Check(handle(runtime) == 0u &&
                  runtime.calls == std::vector<std::string>{
                                       "event", "arena", "read-word"},
              "event one needs arena state one");
        runtime.calls.clear();
        runtime.words[0x70c4u] = 1u;
        Check(handle(runtime) == 1u && runtime.dispatched_kind == 1u &&
                  runtime.calls == std::vector<std::string>{
                                       "event", "arena", "read-word", "switch",
                                       "frame", "dispatch"},
              "event one dispatches after arena gate");
    }
    for (const auto pair : {std::pair{19u, "run"},
                            std::pair{50u, "toggle"}}) {
        Runtime runtime;
        runtime.event_word = pair.first;
        Check(handle(runtime) == 1u &&
                  runtime.calls == std::vector<std::string>{"event", pair.second},
              "run and toggle events");
    }
    for (const auto pair : {std::pair{46u, "window68"},
                            std::pair{47u, "window64"}}) {
        for (const bool visible : {false, true}) {
            Runtime runtime;
            runtime.event_word = pair.first;
            runtime.visible = visible;
            Check(handle(runtime) == 1u &&
                      runtime.calls.size() == (visible ? 3u : 2u) &&
                      runtime.calls[1] == pair.second &&
                      (!visible || runtime.calls[2] ==
                                       (pair.first == 46u ? "skill" : "skill-state")),
                  "skill event visibility and action");
        }
    }
    {
        Runtime runtime;
        runtime.event_word = 15u;
        runtime.active_count = 1;
        Check(handle(runtime) == 0u &&
                  runtime.calls == std::vector<std::string>{
                                       "event", "manager", "count"},
              "player cycle requires two active players");
        runtime.active_count = 2;
        state.WriteWord(0xccu, 0xc000u);
        Check(handle(runtime) == 0u && runtime.calls.back() == "read-word",
              "linked mode blocks player cycle");
        state.WriteWord(0xccu, 0u);
    }
    for (const auto selected : {2u, 3u, 4u}) {
        Runtime runtime;
        runtime.event_word = 15u;
        runtime.players = {0x9000u, 0xa000u, 0xb000u, 0xc000u};
        for (std::uint32_t index = selected; index < 4u; ++index) {
            runtime.players[index] = 0u;
        }
        for (std::uint32_t index = 0u; index < selected; ++index) {
            runtime.words[runtime.players[index] + 0xb08u] = 1u;
            runtime.player_states[runtime.players[index]] = index + 10u;
        }
        runtime.words[0xa000u + 0xa48u] = 0x123u;
        Check(handle(runtime) == 1u &&
                  runtime.sorted_players.size() == selected &&
                  runtime.player_states[0x9000u] == selected + 9u &&
                  runtime.refreshed_action == 0x123u &&
                  runtime.calls[runtime.calls.size() - 5u] ==
                      "set-state:36864" &&
                  runtime.calls[runtime.calls.size() - 4u] == "read-word" &&
                  runtime.calls[runtime.calls.size() - 3u] == "manager" &&
                  runtime.calls[runtime.calls.size() - 2u] == "update-states" &&
                  runtime.calls.back() == "refresh-ui",
              "player cycle uses capacity-based shift and second sorted player");
    }
    {
        Runtime runtime;
        runtime.event_word = 15u;
        runtime.players = {0xc000u, 0u, 0x9000u, 0xa000u};
        runtime.words[0xc000u + 0xb08u] = 1u;
        runtime.words[0x9000u + 0xb08u] = 1u;
        runtime.words[0xa000u + 0xb08u] = 1u;
        runtime.player_states = {{0x9000u, 10u}, {0xa000u, 20u},
                                 {0xc000u, 30u}};
        runtime.words[0xa000u + 0xa48u] = 0x345u;
        state.WriteWord(0xccu, 0xd000u);
        runtime.words[0xd034u] = 2u;
        Check(handle(runtime) == 1u &&
                  runtime.sorted_players ==
                      std::vector<std::uint32_t>{0x9000u, 0xa000u, 0xc000u} &&
                  runtime.player_states[0x9000u] == 30u &&
                  runtime.refreshed_action == 0x345u &&
                  std::count(runtime.calls.begin(), runtime.calls.end(),
                             "manager") == 6,
              "linked mode two permits filtered sorted player cycle");
        state.WriteWord(0xccu, 0u);
    }
    {
        Runtime runtime;
        runtime.words[0x9000u + 0xa48u] = 0x456u;
        pal4::MinimapSceneUiInput::RefreshPlayers(0x5000u, 0x9000u,
                                                  runtime);
        Check(runtime.refreshed_action == 0x456u &&
                  runtime.calls == std::vector<std::string>{
                                       "read-word", "manager", "update-states",
                                       "refresh-ui"},
              "standalone player refresh reads action then updates game UI");
    }
    {
        Runtime runtime;
        runtime.event_word = 15u;
        runtime.words[0x9000u + 0xb08u] = 1u;
        Check(handle(runtime) == 0u && runtime.calls.back() == "sort",
              "too few eligible players are handled safely");
        runtime.throw_in_sort = true;
        bool caught = false;
        try {
            (void)handle(runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught, "sort failure propagates with vector ownership scoped");
    }
}
