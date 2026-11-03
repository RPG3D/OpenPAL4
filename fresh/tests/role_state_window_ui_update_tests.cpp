#include "pal4/role_state_window_ui_update.h"

#include <algorithm>
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

struct Event {
    std::uint32_t tree;
    std::uint32_t command;
    std::uint32_t first;
    std::uint32_t second;
};

struct Runtime final : pal4::IRoleStateWindowPlayerRuntime,
                       pal4::IRoleStateWindowUiRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::array<std::uint32_t, 4> players{};
    std::vector<Event> events;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> matched_entries;
    std::vector<std::uint32_t> refreshed_roles;
    std::uint32_t active_count = 0u;
    std::uint32_t property_count = 0u;
    std::uint32_t cleanup_count = 0u;
    std::uint32_t sort_count = 0u;
    std::uint32_t insert_count = 0u;
    std::uint32_t remove_count = 0u;
    std::uint32_t system_dialog_count = 0u;
    bool throw_in_match = false;

    Runtime() { memory[0x9060u] = 0xa000u; }

    void SetParent(pal4::RoleStateWindowState&, std::uint8_t showing,
                   std::uint32_t parent) override {
        Check(parent == 0x8888u && showing <= 1u,
              "parent and visibility forwarded");
        calls.emplace_back("parent");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    void ProcessEvent(std::uint32_t tree, std::uint32_t command,
                      std::uint32_t first,
                      std::uint32_t second) override {
        events.push_back({tree, command, first, second});
        calls.emplace_back("event");
    }
    std::uint32_t ActivePlayerCount(std::uint32_t manager) override {
        Check(manager == 0x3000u, "active player count manager");
        calls.emplace_back("active-count");
        return active_count;
    }
    std::string ArenaString(std::uint32_t offset) override {
        calls.push_back("arena:" + std::to_string(offset));
        if (offset == 0xe0u) return "arena-path";
        Check(offset == 8u, "arena string offsets");
        return "object-name";
    }
    std::uint32_t ResolveSceneProperty(const std::string& object,
                                       const std::string& arena) override {
        Check(object == "object-name" && arena == "arena-path",
              "trigger variable argument order");
        calls.emplace_back("resolve-scene");
        return 0xb000u;
    }
    void CleanupSceneProperty(std::uint32_t property) override {
        Check(property == 0xb000u, "resolved scene attribute cleanup");
        ++cleanup_count;
        calls.emplace_back("cleanup-scene");
    }
    std::uint32_t ItemPropertyCollection() override {
        calls.emplace_back("item-property");
        return 0x9000u;
    }
    std::uint32_t ItemPropertyCount(std::uint32_t collection) override {
        Check(collection == 0x905cu, "item property count offset");
        return property_count;
    }
    bool MatchesSceneProperty(std::uint32_t scene_property,
                              std::uint32_t entry) override {
        Check(scene_property == 0xb000u && entry >= 0xa000u &&
                  (entry - 0xa000u) % 0x3cu == 0u,
              "scene property and 60-byte entry stride");
        matched_entries.push_back(entry);
        if (throw_in_match) throw std::runtime_error("compare");
        return entry == 0xa03cu || entry == 0xa078u;
    }
    void SortPlayers(std::vector<std::uint32_t>& slots,
                     std::uint32_t comparator) override {
        Check(comparator == 0x4419d0u, "original player comparator");
        ++sort_count;
        calls.emplace_back("sort");
        std::reverse(slots.begin(), slots.end());
    }
    void InsertActive(std::uint32_t array,
                      std::uint32_t member) override {
        Check(array == 0x6134u && member == 0x7028u,
              "active array and role window member offset");
        ++insert_count;
        calls.emplace_back("insert-active");
    }
    void RemoveActive(std::uint32_t array,
                      std::uint32_t member) override {
        Check(array == 0x6134u && member == 0x7028u,
              "removed active member offset");
        ++remove_count;
        calls.emplace_back("remove-active");
    }
    void UpdateSystemDialogVisibility(std::uint32_t manager,
                                      std::uint32_t value) override {
        Check(manager == 0x6000u && value == 1u,
              "system dialog update arguments");
        ++system_dialog_count;
        calls.emplace_back("system-dialog");
    }

    std::uint32_t PlayerManager() override {
        calls.emplace_back("player-manager");
        return 0x3000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0x3000u && index < 4u,
              "player lookup index and manager");
        calls.push_back("player:" + std::to_string(index));
        return players[index];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return memory[address];
    }
    void PreparePlayerData(std::uint32_t data,
                           std::uint32_t role_index) override {
        Check(data >= 0x10654u && role_index < 4u,
              "player preparation receives member role index");
        refreshed_roles.push_back(role_index);
        calls.emplace_back("prepare-player");
    }
    void BeginTextBuffer() override { calls.emplace_back("begin-text"); }
    void EndTextBuffer() override { calls.emplace_back("end-text"); }
    void SetImage(std::uint32_t, std::uint32_t) override {
        calls.emplace_back("portrait");
    }
    std::string ConvertToUtf8(std::uint32_t) override { return "text"; }
    std::int32_t ElementValue(std::uint32_t, std::uint32_t) override {
        return 5;
    }
    std::int32_t BaseValue(std::uint32_t, std::uint32_t) override {
        return 100;
    }
    std::int32_t CurrentValue(std::uint32_t,
                              std::uint32_t field) override {
        return field == 4u ? 99 : 10;
    }
    std::uint32_t TriggerText(std::uint32_t) override { return 0u; }
    std::int32_t NextLevelThreshold(std::uint32_t,
                                    std::int32_t) override {
        Check(false, "level 99 does not query threshold");
        return 0;
    }
    void SetText(std::uint32_t, const std::string&) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    void SetWidth(std::uint32_t, float) override {}
    void SetVisible(std::uint32_t, bool visible) override {
        calls.push_back(visible ? "show-member" : "hide-member");
    }
    void SetImage(std::uint32_t, std::string_view,
                  std::string_view) override {}
    void ClearImage(std::uint32_t) override {}
};

pal4::RoleStateWindowState MakeState() {
    pal4::RoleStateWindowState state;
    state.Write(36u, 0x4444u);
    state.Write(588u, 0xffffffffu);
    state.Write(592u, 0xffffffffu);
    state.Write(596u, 0xffffffffu);
    for (std::uint32_t role = 0u; role < 4u; ++role) {
        const auto base = 44u + role * 0x88u;
        state.Write(base + 8u, role);
        for (std::uint32_t offset : {12u, 20u, 24u, 28u, 32u, 36u,
                                     40u, 48u, 52u, 56u, 60u, 64u,
                                     68u, 72u, 76u, 80u, 84u, 88u,
                                     92u, 96u, 100u})
            state.Write(base + offset, 0x5000u + role * 0x200u + offset);
        for (std::uint32_t i = 0u; i < 4u; ++i)
            state.Write(base + 120u + i * 4u, 0x7000u + i);
        for (std::uint32_t offset : {104u, 108u, 112u})
            state.Write(base + offset, 0x3f800000u);
    }
    return state;
}

void CheckEvents(const Runtime& runtime, bool showing) {
    Check(runtime.events.size() == (showing ? 3u : 2u),
          "show emits three events, hide emits two");
    Check(runtime.events[0].tree == 0x6104u &&
              runtime.events[0].command == 0x8b1394u &&
              runtime.events[0].first == static_cast<std::uint32_t>(showing) &&
              runtime.events[0].second == 0u &&
              runtime.events[1].tree == 0x6104u &&
              runtime.events[1].command == 0x8b13a0u &&
              runtime.events[1].first == static_cast<std::uint32_t>(showing) &&
              runtime.events[1].second == 0x4444u,
          "frame tool and game info event payloads");
    if (showing)
        Check(runtime.events[2].command == 0x8b13a4u &&
                  runtime.events[2].first == 5u &&
                  runtime.events[2].second == 0u,
              "help tips event ID and payload");
}

void TestHide() {
    Runtime runtime;
    runtime.active_count = 3u;
    auto state = MakeState();
    pal4::RoleStateWindowUiUpdate::Update(
        state, 0x7000u, 0u, 0x8888u, runtime, runtime);
    CheckEvents(runtime, false);
    Check(state.Read(592u) == 0u && state.Read(596u) == 3u &&
              state.Read(588u) == 0u && runtime.remove_count == 1u &&
              runtime.insert_count == 0u &&
              runtime.system_dialog_count == 0u &&
              runtime.cleanup_count == 0u && runtime.sort_count == 0u,
          "hide resets root fields and only removes active window");
    Check(runtime.calls.front() == "parent" &&
              runtime.calls.back() == "remove-active",
          "hide call order");
}

void TestShowSceneSelectionWithoutPlayers() {
    Runtime runtime;
    runtime.property_count = 3u;
    auto state = MakeState();
    pal4::RoleStateWindowUiUpdate::Update(
        state, 0x7000u, 1u, 0x8888u, runtime, runtime);
    CheckEvents(runtime, true);
    Check(state.Read(592u) == 0xa03cu &&
              state.Read(596u) == 0u && state.Read(588u) == 0u &&
              runtime.matched_entries ==
                  std::vector<std::uint32_t>{0xa000u, 0xa03cu, 0xa078u},
          "first matching 60-byte item entry becomes selected");
    Check(runtime.cleanup_count == 1u && runtime.sort_count == 1u &&
              runtime.insert_count == 1u &&
              runtime.system_dialog_count == 1u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "hide-member") == 8 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "show-member") == 4,
          "show cleans scene key, hides four members and registers window");
    Check(runtime.calls.back() == "system-dialog" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "cleanup-scene") <
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "sort"),
          "scene cleanup precedes player sorting");
}

void TestSortedPlayerRefresh() {
    Runtime runtime;
    runtime.players = {0u, 0x20000u, 0u, 0x40000u};
    runtime.memory[0x20000u + 0xb08u] = 1u;
    runtime.memory[0x40000u + 0xb08u] = 1u;
    runtime.memory[0x20000u + 0xa48u] = 1u;
    runtime.memory[0x40000u + 0xa48u] = 3u;
    runtime.active_count = 2u;
    auto state = MakeState();
    pal4::RoleStateWindowUiUpdate::Update(
        state, 0x7000u, 1u, 0x8888u, runtime, runtime);
    Check(state.Read(44u) == 3u &&
              state.Read(44u + 0x88u) == 1u &&
              runtime.refreshed_roles ==
                  std::vector<std::uint32_t>{0u, 1u} &&
              state.Read(44u + 2u * 0x88u) == 0u,
          "sorted players refresh successive members using player role IDs");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "prepare-player") == 2 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "end-text") == 2,
          "both active players use rebuilt player-info path");
}

void TestSceneCleanupOnFailure() {
    Runtime runtime;
    runtime.property_count = 1u;
    runtime.throw_in_match = true;
    auto state = MakeState();
    try {
        pal4::RoleStateWindowUiUpdate::Update(
            state, 0x7000u, 1u, 0x8888u, runtime, runtime);
        Check(false, "scene comparison failure expected");
    } catch (const std::runtime_error&) {
        Check(runtime.cleanup_count == 1u &&
                  runtime.events.size() == 2u &&
                  runtime.insert_count == 0u &&
                  state.Read(592u) == 0u,
              "scene attribute cleaned and later UI events skipped");
    }
}

}  // namespace

int main() {
    TestHide();
    TestShowSceneSelectionWithoutPlayers();
    TestSortedPlayerRefresh();
    TestSceneCleanupOnFailure();
}
