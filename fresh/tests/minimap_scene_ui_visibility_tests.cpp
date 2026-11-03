#include "pal4/minimap_scene_ui_visibility.h"

#include <array>
#include <cstdlib>
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

struct Runtime final : pal4::IMinimapSceneUiVisibilityRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> memory;
    std::array<std::uint32_t, 4> players{};
    std::vector<std::pair<std::uint32_t, pal4::MinimapUiPosition>> positions;
    std::uint32_t parent_value = 0;
    std::uint32_t component_value = 0;
    std::uint32_t chosen_player = 0;
    std::uint32_t callback = 0xffffffffu;
    std::uint32_t allocation = 0x8000u;
    std::uint32_t active_array = 0;
    std::uint32_t active_component = 0;
    std::uint32_t result = 0;
    std::int32_t game_entry = 1;
    std::uint8_t game_flag = 0xffu;
    bool timer_exception_message = false;
    bool throw_on_remove = false;
    bool throw_on_add = false;
    bool allow_jump_hint_visible = false;
    int timer_begins = 0;
    int timer_ends = 0;
    int player_manager_calls = 0;

    Runtime() {
        memory[0x9000u + 0x2c50u] = 1u;
        memory[0x7000u + 0x1a0u] = 0x7800u;
    }

    void SetParent(std::uint32_t owner, std::uint32_t visible,
                   std::uint32_t parent) override {
        Check(owner == 0x5000u && parent == 0x6000u, "parent arguments");
        parent_value = visible;
        calls.emplace_back("parent");
    }
    void CloseUtf8Window(std::uint32_t address) override {
        Check(address == 0x506cu, "UTF-8 member offset");
        calls.emplace_back("close");
    }
    std::int32_t GameEntry(std::uint32_t index) override {
        Check(index == 26u, "game entry index");
        calls.emplace_back("game-entry");
        return game_entry;
    }
    void SetGameEntry(std::uint32_t index,
                      std::uint8_t value) override {
        Check(index == 26u, "written game entry index");
        game_flag = value;
        calls.emplace_back("game-write");
    }
    void SetComponentVisible(std::uint32_t component,
                             std::uint32_t visible,
                             std::uint32_t root_window) override {
        Check(component == 0x502cu && root_window == 0x1000u,
              "component and root arguments");
        component_value = visible;
        calls.emplace_back("component");
    }
    void SetClippedByParent(std::uint32_t window, bool clipped) override {
        Check(!clipped, "clipping is disabled");
        calls.push_back(window == 0x2000u ? "overlay-clip" : "root-clip");
    }
    std::pair<std::int32_t, std::int32_t> ConfigSize() override {
        calls.emplace_back("config");
        return {1280, 720};
    }
    std::uint32_t FrameManager() override {
        return 0x7000u;
    }
    pal4::MinimapUiPosition ConvertCoordinates(
        std::uint32_t manager, pal4::MinimapUiPosition input) override {
        Check(manager == 0x7000u, "coordinate manager");
        if (input.x == 1280.0f && input.y == 0.0f) {
            calls.emplace_back("convert-root");
            return {12.0f, 0.5f};
        }
        Check(input.x == 0.0f && input.y == 720.0f,
              "overlay conversion input");
        calls.emplace_back("convert-overlay");
        return {1.0f, 9.0f};
    }
    float Width(std::uint32_t window, std::uint32_t mode) override {
        Check(window == 0x1000u && mode == 1u, "root width query");
        return 3.0f;
    }
    float Height(std::uint32_t window, std::uint32_t mode) override {
        Check(window == 0x2000u && mode == 1u, "overlay height query");
        return 2.0f;
    }
    void SetPosition(std::uint32_t window, std::uint32_t mode,
                     pal4::MinimapUiPosition position) override {
        Check(mode == 1u, "position mode");
        positions.emplace_back(window, position);
        calls.push_back(window == 0x1000u ? "root-position" : "overlay-position");
    }
    std::uint32_t SetPositionRaw(std::uint32_t window,
                                 std::uint32_t mode,
                                 std::uint32_t position_address) override {
        Check(window == 0x3000u && mode == 1u &&
                  position_address == 0x8800u,
              "jump hint raw position forwarding");
        calls.emplace_back("raw-position");
        return 0x99u;
    }
    void SetAlwaysOnTop(std::uint32_t window, bool enabled) override {
        Check(window == 0x2000u && enabled, "overlay topmost");
        calls.emplace_back("topmost");
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check((window == 0x3000u && (!visible || allow_jump_hint_visible)) ||
                  (window == 0x4000u && visible) ||
                  window == 0x6000u,
              "prompt and control visibility");
        if (window == 0x6000u) {
            calls.push_back(visible ? "linked-visible" : "linked-hidden");
        } else {
            calls.push_back(visible ? "control-visible" : "prompt-hidden");
        }
    }
    std::uint32_t CreateProgressBar(std::uint32_t manager) override {
        Check(manager == 0x7000u, "progress manager");
        return 0x7100u;
    }
    void SetProgressVisible(std::uint32_t progress, bool visible) override {
        Check(progress == 0x7100u && !visible, "progress hidden");
        calls.emplace_back("progress-hidden");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        memory[address] = value;
        calls.emplace_back("write");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return memory[address];
    }
    void UpdateCharacterMovement(std::uint32_t owner) override {
        Check(owner == 0x5000u, "movement owner");
        calls.emplace_back("movement");
    }
    std::uint32_t PlayerManager() override {
        ++player_manager_calls;
        return 0x9000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0x9000u && index < players.size(), "player lookup");
        calls.push_back("player-" + std::to_string(index));
        return players[index];
    }
    bool PlayerStateBlocked(std::uint32_t address) override {
        calls.emplace_back("state");
        return address == 0xb654u;
    }
    void UpdatePlayerStates(std::uint32_t owner,
                            std::uint32_t player) override {
        Check(owner == 0x5000u, "player update owner");
        chosen_player = player;
        calls.emplace_back("player-update");
    }
    void UpdatePlayerAndGameStates(std::uint32_t owner) override {
        Check(owner == 0x5000u, "game state owner");
        calls.emplace_back("game-update");
    }
    void UpdateVisibilityAndMinimap(std::uint32_t owner) override {
        Check(owner == 0x5000u, "minimap owner");
        calls.emplace_back("minimap-update");
    }
    void BeginTimerName(std::string_view name,
                        bool exception_message) override {
        Check(name == "FlashRoleButton", "timer name");
        ++timer_begins;
        timer_exception_message = exception_message;
        calls.emplace_back("timer-begin");
    }
    void RemoveTimerByName(std::uint32_t queue,
                           std::string_view name) override {
        Check(queue == 0x7800u && name == "FlashRoleButton",
              "timer removal arguments");
        calls.emplace_back("timer-remove");
        if (throw_on_remove) throw std::runtime_error("remove");
    }
    void EndTimerName() override {
        ++timer_ends;
        calls.emplace_back("timer-end");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 16u, "timer callback allocation");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t CreateTimerEntry(std::string_view name,
                                   std::uint32_t period_bits,
                                   std::uint32_t value) override {
        Check(name == "FlashRoleButton" && period_bits == 0x3e99999au,
              "timer period and name");
        callback = value;
        calls.emplace_back("timer-create");
        return 0xa000u;
    }
    void AddTimerEntry(std::uint32_t queue,
                       std::uint32_t entry) override {
        Check(queue == 0x7800u && entry == 0xa000u,
              "timer insertion arguments");
        calls.emplace_back("timer-add");
        if (throw_on_add) throw std::runtime_error("add");
    }
    void DestroyTimerEntry(std::uint32_t entry) override {
        Check(entry == 0xa000u, "timer cleanup entry");
        calls.emplace_back("timer-destroy");
    }
    std::uint32_t InsertActive(std::uint32_t array,
                               std::uint32_t component) override {
        active_array = array;
        active_component = component;
        calls.emplace_back("active-insert");
        return 0x55u;
    }
    std::uint32_t RemoveActive(std::uint32_t array,
                               std::uint32_t component) override {
        active_array = array;
        active_component = component;
        calls.emplace_back("active-remove");
        return 0x66u;
    }
};

pal4::MinimapSceneUiState MakeState() {
    pal4::MinimapSceneUiState state;
    state.WriteWord(0x24u, 0x1000u);
    state.WriteWord(0x50u, 0x2000u);
    state.WriteWord(0xc0u, 0x3000u);
    state.WriteWord(0x68u, 0x4000u);
    state.WriteByte(0xf6u, 1u);
    return state;
}

void CheckJumpHint() {
    const auto state = MakeState();
    Runtime runtime;
    runtime.allow_jump_hint_visible = true;
    Check(pal4::MinimapSceneUiVisibility::SetJumpHint(
              state, true, 0x8800u, runtime) == 0x99u &&
              runtime.calls == std::vector<std::string>{
                                   "control-visible", "raw-position"},
          "jump hint visibility precedes raw position and returns result");
    for (const auto value : {-1, 0, 1}) {
        Runtime forwarded;
        forwarded.allow_jump_hint_visible = true;
        Check(pal4::MinimapSceneUiVisibility::SetJumpHintFromArgs(
                  state, {value, 0x8800u}, forwarded) == 1u &&
                  forwarded.calls == std::vector<std::string>{
                                         value > 0 ? "control-visible"
                                                   : "prompt-hidden",
                                         "raw-position"},
              "signed visibility argument gates forwarded jump hint");
    }
}

std::size_t Position(const Runtime& runtime, const std::string& call) {
    for (std::size_t index = 0; index < runtime.calls.size(); ++index) {
        if (runtime.calls[index] == call) return index;
    }
    return runtime.calls.size();
}

std::size_t LastPosition(const Runtime& runtime, const std::string& call) {
    for (std::size_t index = runtime.calls.size(); index > 0u; --index) {
        if (runtime.calls[index - 1u] == call) return index - 1u;
    }
    return runtime.calls.size();
}

void CheckShow() {
    auto state = MakeState();
    Runtime runtime;
    runtime.players = {0u, 0xb000u, 0xc000u, 0xd000u};
    runtime.memory[0xb000u + 0xb08u] = 1u;
    runtime.memory[0xc000u + 0xb08u] = 0u;
    runtime.memory[0xd000u + 0xb08u] = 1u;
    Check(pal4::MinimapSceneUiVisibility::Update(
              state, 0x5000u, 0x101u, 0x6000u, runtime) == 0x55u,
          "show returns active-list insertion result");
    Check(runtime.parent_value == 0x101u &&
              runtime.component_value == 0x101u &&
              state.ReadByte(0xf6u) == 0u,
          "full visibility argument forwarded and transient byte cleared");
    Check(runtime.game_flag == 1u && state.ReadByte(0xf7u) == 1u &&
              runtime.memory[0x7144u] == 1u &&
              Position(runtime, "game-write") <
                  Position(runtime, "component"),
          "positive entry enables game flag and manager field");
    Check(runtime.positions.size() == 2u &&
              runtime.positions[0].first == 0x1000u &&
              runtime.positions[0].second.x == 9.0f &&
              runtime.positions[0].second.y == 0.5f &&
              runtime.positions[1].first == 0x2000u &&
              runtime.positions[1].second.x == -8.0f &&
              runtime.positions[1].second.y == 7.0f,
          "root and overlay coordinate calculations");
    Check(runtime.chosen_player == 0xd000u &&
              Position(runtime, "player-3") < Position(runtime, "player-update") &&
              Position(runtime, "player-update") < Position(runtime, "game-update"),
          "player scan short circuits inactive candidates");
    Check(runtime.callback == 0x8000u &&
              runtime.memory[0x8000u] == 0x843b20u &&
              runtime.memory[0x8004u] == 0x5000u &&
              runtime.memory[0x8008u] == 0x479470u &&
              runtime.memory[0x800cu] == 0u,
          "timer callback layout");
    Check(runtime.timer_begins == 2 && runtime.timer_ends == 2 &&
              !runtime.timer_exception_message &&
              runtime.active_array == 0x7134u &&
              runtime.active_component == 0x5028u &&
              Position(runtime, "timer-remove") < Position(runtime, "timer-create") &&
              Position(runtime, "timer-destroy") < Position(runtime, "active-insert"),
          "timer replacement and active-list order");
}

void CheckHideAndLowByte() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::MinimapSceneUiVisibility::Update(
              state, 0x5000u, 0x100u, 0x6000u, runtime) == 0x66u,
          "zero low byte takes hide branch");
    Check(runtime.parent_value == 0x100u && runtime.component_value == 0x100u &&
              runtime.positions.empty() && runtime.player_manager_calls == 0 &&
              Position(runtime, "game-entry") == runtime.calls.size() &&
              runtime.timer_exception_message &&
              runtime.active_array == 0x7134u &&
              runtime.active_component == 0x5028u &&
              Position(runtime, "timer-end") < Position(runtime, "active-remove"),
          "hide skips show-only work and removes timer before active entry");
}

void CheckPlayerBranches() {
    auto state = MakeState();
    Runtime current;
    current.game_entry = 0;
    current.players = {0x9000u + 2836u, 0u, 0u, 0u};
    current.memory[current.players[0] + 0xb08u] = 1u;
    (void)pal4::MinimapSceneUiVisibility::Update(
        state, 0x5000u, 1u, 0x6000u, current);
    Check(current.game_flag == 0u && state.ReadByte(0xf7u) == 0u &&
              current.chosen_player == 0u &&
              Position(current, "player-3") < Position(current, "game-update"),
          "current player is excluded and scan reaches four candidates");

    Runtime blocked;
    blocked.players = {0xb000u, 0u, 0u, 0u};
    blocked.memory[0xb000u + 0xb08u] = 1u;
    (void)pal4::MinimapSceneUiVisibility::Update(
        state, 0x5000u, 1u, 0x6000u, blocked);
    Check(blocked.chosen_player == 0u &&
              Position(blocked, "state") < Position(blocked, "player-1"),
          "blocked state rejects candidate before continuing scan");
}

void CheckGameFlagEntry() {
    auto state = MakeState();
    Runtime runtime;
    pal4::MinimapSceneUiVisibility::StoreGameFlag(
        state, 0x5000u, 0x80u, runtime);
    Check(state.ReadByte(0xf7u) == 0x80u && runtime.game_flag == 0x80u &&
              runtime.calls == std::vector<std::string>{"game-write"},
          "flag helper writes raw byte to object and GameData before refresh");
}

void CheckLinkedVisibility() {
    auto state = MakeState();
    state.WriteWord(0xccu, 0xa000u);
    state.WriteWord(0x5cu, 0x6000u);
    Runtime runtime;
    state.WriteByte(0xf7u, 0x80u);
    for (auto mode : {0u, 1u, 2u}) {
        runtime.memory[0xa034u] = mode;
        pal4::MinimapSceneUiVisibility::UpdateVisibilityBasedOnState(
            state, runtime);
        Check(runtime.calls.back() == "linked-hidden",
              "states zero/one and state two mask off high-only flag");
    }
    runtime.memory[0xa034u] = 3u;
    pal4::MinimapSceneUiVisibility::UpdateVisibilityBasedOnState(
        state, runtime);
    Check(runtime.calls.back() == "linked-visible",
          "state three preserves nonzero high flag");
    state.WriteByte(0xf7u, 3u);
    runtime.memory[0xa034u] = 2u;
    pal4::MinimapSceneUiVisibility::UpdateVisibilityBasedOnState(
        state, runtime);
    Check(runtime.calls.back() == "linked-visible",
          "state two uses low bit of game flag");
    state.WriteWord(0xccu, 0u);
    const auto prior = runtime.calls.size();
    pal4::MinimapSceneUiVisibility::UpdateVisibilityBasedOnState(
        state, runtime);
    Check(runtime.calls.size() == prior,
          "missing linked object performs no visibility operation");
}

void CheckTimerFailures() {
    auto state = MakeState();
    Runtime null_allocation;
    null_allocation.allocation = 0u;
    (void)pal4::MinimapSceneUiVisibility::Update(
        state, 0x5000u, 1u, 0x6000u, null_allocation);
    Check(null_allocation.callback == 0u &&
              Position(null_allocation, "timer-create") <
                  Position(null_allocation, "timer-add"),
          "allocation failure still constructs a timer with null callback");

    Runtime remove_failure;
    remove_failure.throw_on_remove = true;
    bool caught = false;
    try {
        (void)pal4::MinimapSceneUiVisibility::Update(
            state, 0x5000u, 1u, 0x6000u, remove_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && remove_failure.timer_begins == 1 &&
              remove_failure.timer_ends == 1 &&
              Position(remove_failure, "timer-create") == remove_failure.calls.size(),
          "timer name is released after removal failure");

    Runtime add_failure;
    add_failure.throw_on_add = true;
    caught = false;
    try {
        (void)pal4::MinimapSceneUiVisibility::Update(
            state, 0x5000u, 1u, 0x6000u, add_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && add_failure.timer_begins == 2 &&
              add_failure.timer_ends == 2 &&
              Position(add_failure, "timer-add") <
                  Position(add_failure, "timer-destroy") &&
              Position(add_failure, "timer-destroy") <
                  LastPosition(add_failure, "timer-end") &&
              Position(add_failure, "active-insert") == add_failure.calls.size(),
          "timer entry and name are released after insertion failure");
}

}  // namespace

int main() {
    CheckShow();
    CheckHideAndLowByte();
    CheckPlayerBranches();
    CheckGameFlagEntry();
    CheckLinkedVisibility();
    CheckJumpHint();
    CheckTimerFailures();
}
