#include "pal4/equipment_window_callbacks.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentWindowCallbacksRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t row{0x8000u};
    std::uint32_t item{0x9000u};
    std::uint32_t player{0xa000u};
    std::unordered_map<std::uint32_t, std::uint32_t> previous;
    std::unordered_map<std::uint32_t, std::uint32_t> next;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
        words[address] = value;
    }
    std::uint32_t HoveredRow(std::uint32_t list) override {
        Check(list == 0x4000u, "hovered row list");
        calls.emplace_back("hovered-row");
        return row;
    }
    std::uint32_t InventoryItem(std::uint32_t id,
                                std::uint32_t kind) override {
        Check(id == 0x55u && kind == 7u, "inventory lookup key and kind");
        calls.emplace_back("inventory");
        return item;
    }
    void HandleItemClick(pal4::EquipmentWindowState&,
                         std::uint32_t data) override {
        Check(data == 0x500cu, "mouse click passes event +12");
        calls.emplace_back("item-click");
    }
    void UpdateItemDetails(pal4::EquipmentWindowState&,
                           std::uint32_t value) override {
        Check(value == item || value == 0u, "details use hovered item");
        calls.emplace_back("details");
    }
    void UpdateItemModel(pal4::EquipmentWindowState&,
                         std::uint32_t value) override {
        Check(value == item, "model uses hovered item");
        calls.emplace_back("model");
    }
    void UpdateList(pal4::EquipmentWindowState& state,
                    std::uint32_t category,
                    std::uint32_t character) override {
        Check(character == state.Read(0xf4u), "list uses selected character");
        calls.emplace_back("list:" + std::to_string(category));
    }
    void UpdateDisplay(pal4::EquipmentWindowState& state,
                       std::uint32_t value,
                       std::uint32_t character) override {
        Check(value == item || value == 0u,
              "display uses hovered item");
        Check(character == state.Read(0xf4u),
              "display uses selected character");
        calls.emplace_back("display");
    }
    void SetImage(std::uint32_t window, std::uint32_t image) override {
        Check(window == 0xb000u && image == 0xc002u,
              "category image selection");
        calls.emplace_back("image");
    }
    void InsertMember(std::uint32_t offset) override {
        calls.emplace_back("insert:" + std::to_string(offset));
    }
    void RemoveMember(std::uint32_t offset) override {
        calls.emplace_back("remove:" + std::to_string(offset));
    }
    void RefreshSecondaryMember(std::uint32_t offset) override {
        Check(offset == 0x118u, "refresh secondary member");
        calls.emplace_back("refresh-secondary");
    }
    std::uint32_t PreviousCharacterIndex(std::uint32_t value) override {
        calls.emplace_back("previous:" + std::to_string(value));
        return previous.at(value);
    }
    std::uint32_t NextCharacterIndex(std::uint32_t value) override {
        calls.emplace_back("next:" + std::to_string(value));
        return next.at(value);
    }
    std::uint32_t Player(std::uint32_t character) override {
        calls.emplace_back("player:" + std::to_string(character));
        return player;
    }
    void PlaySound(std::string_view file, std::uint32_t mode) override {
        Check(file == "wa13.wav" && mode == 1u,
              "unequip retains inventory and uses equipment sound, not discard sound");
        calls.emplace_back("sound");
    }
    void RefreshPlayer(std::uint32_t value) override {
        Check(value == player, "refresh selected player");
        calls.emplace_back("refresh-player");
    }
    std::uint32_t HoverRowIndex(std::uint32_t list) override {
        Check(list == 0x4000u, "row index list");
        calls.emplace_back("row-index");
        return 3u;
    }
    float ScrollPosition(std::uint32_t list) override {
        Check(list == 0x4000u, "scroll position list");
        calls.emplace_back("scroll-position");
        return 0.75f;
    }
    void SetScrollPosition(std::uint32_t list, float position) override {
        Check(list == 0x4000u && position == 0.75f,
              "restore scroll position");
        calls.emplace_back("restore-scroll");
    }
    void SetHoverRowIndex(std::uint32_t list,
                          std::uint32_t value) override {
        Check(list == 0x4000u && value == 3u, "restore hover row");
        calls.emplace_back("restore-row");
    }
};

pal4::EquipmentWindowState MakeState() {
    pal4::EquipmentWindowState state{};
    state.Write(0x40u, 0x4000u);
    state.Write(0xb0u, 0xb000u);
    state.Write(0xd0u, 0xc002u);
    state.Write(0xf0u, 2u);
    state.Write(0xf4u, 1u);
    return state;
}

Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[0x8138u] = 0x55u;
    runtime.words[0x5008u] = 0x6000u;
    runtime.words[0x6118u] = 2u;
    runtime.words[0xa000u + 0xb08u] = 1u;
    return runtime;
}

void TestHoverAndClick() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    Check(pal4::EquipmentWindowCallbacks::HoveredItem(state, runtime) ==
              0x9000u, "hover resolves inventory item");
    Check(runtime.calls == std::vector<std::string>{
              "hovered-row", "read:33080", "inventory"},
          "hover lookup order");
    runtime.calls.clear();
    runtime.row = 0u;
    Check(pal4::EquipmentWindowCallbacks::HoveredItem(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"hovered-row"},
          "empty hover returns without inventory lookup");
    Check(pal4::EquipmentWindowCallbacks::HandleHoverChange(state, runtime),
          "hover callback returns true");
    Check(runtime.calls == std::vector<std::string>{
              "hovered-row", "hovered-row", "details"},
          "empty hover clears details without model update");
    runtime.calls.clear();
    runtime.row = 0x8000u;
    Check(pal4::EquipmentWindowCallbacks::HandleHoverChange(state, runtime),
          "item hover returns true");
    Check(runtime.calls == std::vector<std::string>{
              "hovered-row", "read:33080", "inventory", "details", "model"},
          "item hover updates details then model");
    runtime.calls.clear();
    Check(pal4::EquipmentWindowCallbacks::HandleMouseClick(
              state, 0x5000u, runtime) &&
              runtime.calls == std::vector<std::string>{"item-click"},
          "mouse click forwards event payload");
}

void TestCharacterIndexCycle() {
    for (std::int32_t current = 0; current < 4; ++current) {
        Check(pal4::EquipmentCharacterIndex::Next(current) ==
                  (current == 3 ? 0 : current + 1),
              "next index wraps after character three");
        Check(pal4::EquipmentCharacterIndex::Previous(current) ==
                  (current == 0 ? 3 : current - 1),
              "previous index wraps before character zero");
    }
    Check(pal4::EquipmentCharacterIndex::Next(4) == 0 &&
              pal4::EquipmentCharacterIndex::Previous(4) == 3,
          "sentinel transitions to first or last character");
    pal4::EquipmentWindowCallbacksRuntimeStub runtime;
    Check(runtime.NextCharacterIndex(3u) == 0u &&
              runtime.PreviousCharacterIndex(0u) == 3u,
          "runtime stub uses reconstructed helpers");
    Check(pal4::EquipmentCharacterIndex::Next(
              std::numeric_limits<std::int32_t>::max()) ==
              std::numeric_limits<std::int32_t>::min(),
          "next preserves x86 signed overflow behavior");
    Check(pal4::EquipmentCharacterIndex::Previous(
              std::numeric_limits<std::int32_t>::min()) ==
              std::numeric_limits<std::int32_t>::max(),
          "previous preserves x86 signed underflow behavior");
}

void TestCategorySelection() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    Check(pal4::EquipmentWindowCallbacks::HandleCharacterClick(
              state, 0x5000u, runtime), "character click returns true");
    Check(runtime.calls == std::vector<std::string>{
              "read:20488", "read:24856", "list:2", "hovered-row",
              "read:33080", "inventory", "display", "image", "insert:268"},
          "category click reads event window and refreshes UI in order");
}

void TestCharacterNavigation() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    runtime.previous = {{1u, 0u}, {0u, 4u}};
    runtime.player = 0u;
    Check(pal4::EquipmentWindowCallbacks::SelectPreviousCharacter(
              state, runtime) && state.Read(0xf4u) == 4u,
          "previous navigation skips missing player and accepts sentinel");
    Check(runtime.calls == std::vector<std::string>{
              "previous:1", "player:0", "previous:0", "remove:268",
              "remove:280", "refresh-secondary", "insert:268", "list:2",
              "hovered-row", "read:33080", "inventory", "display"},
          "previous navigation call order");

    state = MakeState();
    runtime = MakeRuntime();
    runtime.next = {{1u, 2u}, {2u, 3u}};
    runtime.words[runtime.player + 0xb08u] = 1u;
    Check(pal4::EquipmentWindowCallbacks::SelectNextCharacter(
              state, runtime) && state.Read(0xf4u) == 2u,
          "next navigation selects active character");
    Check(runtime.calls.front() == "next:1" &&
              runtime.calls.back() == "display", "next uses next-index helper");

    state = MakeState();
    runtime = MakeRuntime();
    runtime.next = {{1u, 1u}};
    Check(pal4::EquipmentWindowCallbacks::SelectNextCharacter(
              state, runtime) &&
              runtime.calls == std::vector<std::string>{"next:1"},
          "same-index navigation exits without refreshing");
}

void TestUnequip() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    Check(pal4::EquipmentWindowCallbacks::Unequip(
              state, 0x5000u, runtime), "unequip returns true");
    Check(runtime.calls == std::vector<std::string>{
              "read:20488", "read:24856", "player:1", "sound",
              "write:43304:0", "refresh-player", "hovered-row",
              "read:33080", "inventory", "display", "row-index",
              "scroll-position", "list:2", "restore-scroll", "restore-row"},
          "unequip clears slot then restores list view");
    Check(runtime.words[0xa928u] == 0u, "unequip zeroes slot two");

    runtime = MakeRuntime();
    runtime.player = 0u;
    Check(pal4::EquipmentWindowCallbacks::Unequip(
              state, 0x5000u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "read:20488", "read:24856", "player:1"},
          "missing player avoids UI mutation");

    runtime = MakeRuntime();
    runtime.words[0x6118u] = 6u;
    Check(pal4::EquipmentWindowCallbacks::Unequip(
              state, 0x5000u, runtime), "out-of-range slot still refreshes");
    Check(std::find_if(runtime.calls.begin(), runtime.calls.end(),
                       [](const auto& call) {
                           return call.starts_with("write:");
                       }) == runtime.calls.end(),
          "out-of-range slot never writes player equipment");
}

void TestCharacterSelectionEvent() {
    for (const auto [event_value, category] :
         {std::pair{2u, 5u}, std::pair{3u, 0u}, std::pair{7u, 4u}}) {
        auto state = MakeState();
        state.Write(0xc8u + category * 4u, 0xc002u);
        auto runtime = MakeRuntime();
        runtime.words[0x5010u] = event_value;
        Check(pal4::EquipmentWindowCallbacks::HandleCharacterSelection(
                  state, 0x5000u, runtime),
              "mapped selection event succeeds");
        Check(runtime.calls[0] == "read:20496" &&
                  runtime.calls[1] ==
                      "list:" + std::to_string(category) &&
                  runtime.calls.back() == "insert:268",
              "selection event maps to category and refreshes UI");
    }
    for (const auto event_value : {0u, 1u, 8u}) {
        auto state = MakeState();
        auto runtime = MakeRuntime();
        runtime.words[0x5010u] = event_value;
        Check(!pal4::EquipmentWindowCallbacks::HandleCharacterSelection(
                  state, 0x5000u, runtime) &&
                  runtime.calls == std::vector<std::string>{"read:20496"},
              "unmapped selection event returns false without UI changes");
    }
}

}  // namespace

int main() {
    TestCharacterIndexCycle();
    TestHoverAndClick();
    TestCategorySelection();
    TestCharacterNavigation();
    TestUnequip();
    TestCharacterSelectionEvent();
}
