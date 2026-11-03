#include "pal4/magic_window_visibility.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Event {
    std::uint32_t type{};
    std::uint32_t first{};
    std::uint32_t second{};
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::MagicWindowVisibilityRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> entry_ids;
    std::vector<std::pair<std::uint32_t, bool>> enabled;
    std::vector<std::pair<std::uint32_t, bool>> selected;
    std::vector<std::int32_t> next_inputs;
    std::vector<std::int32_t> player_queries;
    std::vector<std::int32_t> next_results;
    std::vector<Event> events;
    std::uint32_t parent{};
    std::uint8_t showing{};
    std::uint32_t inserted{};
    std::uint32_t removed{};
    bool entry19{};
    bool entry20{};
    std::int32_t active_index{2};

    bool HasGameDataEntry(std::uint32_t id) override {
        entry_ids.push_back(id);
        calls.push_back("entry:" + std::to_string(id));
        return id == 19u ? entry19 : entry20;
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        enabled.emplace_back(window, value);
        calls.push_back("enabled:" + std::to_string(window));
    }
    void SetParent(std::uint32_t object, std::uint8_t value,
                   std::uint32_t new_parent) override {
        Check(object == 0x3000u, "dialog object");
        showing = value;
        parent = new_parent;
        calls.push_back("parent");
    }
    void SetSelected(std::uint32_t button, bool value) override {
        selected.emplace_back(button, value);
        calls.push_back("selected:" + std::to_string(button));
    }
    void SetStuntPanelVisible(std::uint32_t object, bool value) override {
        Check(object == 0x3000u && !value, "stunt panel hidden when opening");
        calls.push_back("stunt-panel");
    }
    std::int32_t NextCharacterIndex(std::int32_t current) override {
        next_inputs.push_back(current);
        calls.push_back("next");
        if (next_results.empty()) return 4;
        const auto result = next_results.front();
        next_results.erase(next_results.begin());
        return result;
    }
    bool HasActivePlayer(std::int32_t index) override {
        player_queries.push_back(index);
        calls.push_back("player:" + std::to_string(index));
        return index == active_index;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x3024u, "owner event reads object +0x24");
        calls.push_back("read-owner");
        return 0x7788u;
    }
    void ProcessEvent(std::uint32_t type, std::uint32_t first,
                      std::uint32_t second) override {
        events.push_back({type, first, second});
        calls.push_back("event:" + std::to_string(type));
    }
    void InsertActive(std::uint32_t member) override {
        inserted = member;
        calls.push_back("insert");
    }
    void RemoveActive(std::uint32_t member) override {
        removed = member;
        calls.push_back("remove");
    }
    void SelectStunt(std::uint32_t object) override {
        Check(object == 0x3000u, "initial stunt selection object");
        calls.push_back("select-stunt");
    }
    void UpdateSystemDialogVisibility(std::uint32_t dialog) override {
        Check(dialog == 4u, "system dialog four");
        calls.push_back("system-dialog");
    }
};

pal4::MagicWindowPanelState Panel() {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t offset = 0; offset < 0x170u; offset += 4u)
        panel.At(offset) = 0x1000u + offset;
    return panel;
}

void CheckEvents(const Runtime& runtime, std::uint8_t showing) {
    const auto expected = showing
        ? std::vector<Event>{{0x8b1394u, showing, 0u},
                             {0x8b13a0u, showing, 0x7788u},
                             {0x8b13a4u, 8u, 0u}}
        : std::vector<Event>{{0x8b1394u, 0u, 0u},
                             {0x8b13a0u, 0u, 0x7788u}};
    Check(runtime.events == expected, "UI event types, payloads and order");
}

void TestOpenWithActivePlayer() {
    Runtime runtime;
    runtime.entry19 = true;
    runtime.entry20 = false;
    runtime.next_results = {0, 1, 2};
    pal4::MagicWindowState state;
    state.selected_character_index = 3;
    state.category_position = 1u;
    pal4::MagicWindowVisibility::Set(0x3000u, 1u, 0x5000u, state, Panel(), runtime);
    Check(runtime.entry_ids == std::vector<std::uint32_t>{19u, 20u} &&
              runtime.enabled.size() == 6u, "two GameData gates and six category buttons");
    Check(runtime.enabled[0] == std::pair<std::uint32_t, bool>{0x10a8u, true},
          "stunt category uses entry 19");
    for (std::uint32_t index = 1; index < 6u; ++index)
        Check(runtime.enabled[index] ==
                  std::pair<std::uint32_t, bool>{0x10a8u + 4u * index, false},
              "five magic categories share entry 20");
    Check(runtime.showing == 1u && runtime.parent == 0x5000u &&
              runtime.selected.size() == 6u, "parent and six selection clears");
    for (std::uint32_t index = 0; index < 6u; ++index)
        Check(runtime.selected[index] ==
                  std::pair<std::uint32_t, bool>{0x10a8u + 4u * index, false},
              "category selection reset order");
    Check(runtime.next_inputs == std::vector<std::int32_t>{4, 0, 1} &&
              runtime.player_queries == std::vector<std::int32_t>{0, 1, 2} &&
              state.selected_character_index == 2 && state.category_position == 0u,
          "search skips unavailable players then initializes stunt category");
    CheckEvents(runtime, 1u);
    Check(runtime.inserted == 0x3028u && runtime.removed == 0u &&
              runtime.calls[runtime.calls.size() - 3] == "insert" &&
              runtime.calls[runtime.calls.size() - 2] == "select-stunt" &&
              runtime.calls.back() == "system-dialog", "open tail order");
    const auto parent_call = std::find(runtime.calls.begin(), runtime.calls.end(), "parent");
    const auto first_event = std::find(runtime.calls.begin(), runtime.calls.end(), "event:9114516");
    Check(parent_call != runtime.calls.end() && first_event != runtime.calls.end() &&
              parent_call < first_event, "parent set before event dispatch");
}

void TestOpenWithoutActivePlayer() {
    Runtime runtime;
    runtime.entry20 = true;
    runtime.next_results = {4};
    pal4::MagicWindowState state;
    pal4::MagicWindowVisibility::Set(0x3000u, 2u, 0, state, Panel(), runtime);
    Check(runtime.player_queries.empty() && state.selected_character_index == 4 &&
              state.category_position == 0u, "sentinel avoids player lookup");
    Check(runtime.enabled[0].second == false && runtime.enabled[1].second &&
              runtime.enabled[5].second, "independent category gates");
    CheckEvents(runtime, 2u);
}

void TestClose() {
    Runtime runtime;
    pal4::MagicWindowState state;
    state.selected_character_index = 2;
    state.category_position = 3u;
    pal4::MagicWindowVisibility::Set(0x3000u, 0u, 0x6000u, state, Panel(), runtime);
    Check(runtime.calls.front() == "parent" && runtime.calls.back() == "remove" &&
              runtime.removed == 0x3028u && runtime.inserted == 0u,
          "close parent and active-list removal");
    Check(runtime.entry_ids.empty() && runtime.enabled.empty() &&
              runtime.selected.empty() && runtime.player_queries.empty() &&
              state.selected_character_index == 2 && state.category_position == 3u,
          "close leaves selection and category unchanged");
    CheckEvents(runtime, 0u);
}

}  // namespace

int main() {
    TestOpenWithActivePlayer();
    TestOpenWithoutActivePlayer();
    TestClose();
}
