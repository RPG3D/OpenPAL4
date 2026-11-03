#include "pal4/equipment_window_actions.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentWindowActionsRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t player = 0x9000u;
    std::uint32_t item = 0xa000u;
    std::uint32_t slot = 2u;
    std::uint32_t callback = 0xb000u;
    std::vector<std::uint32_t> hovered;
    std::size_t hover_index{};
    std::uint32_t rows = 2u;
    bool throw_dialog{};

    std::uint32_t Player(std::uint32_t index) override {
        Check(index == 1u || index == 4u, "player index");
        calls.emplace_back("player");
        return player;
    }
    std::uint32_t InventoryItem(std::uint32_t id,
                                std::uint32_t kind) override {
        Check(id == 0x123u && kind == 7u, "inventory lookup arguments");
        calls.emplace_back("item");
        return item;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == player + 0x928u && value == 0x777u,
              "equipment slot receives inventory field +0x190");
        calls.emplace_back("write");
    }
    void PlaySound(std::string_view file, std::uint32_t mode) override {
        Check(mode == 1u && (file == "wa13.wav" || file == "wa07.wav"),
              "sound file and mode");
        calls.emplace_back(std::string(file));
    }
    std::uint32_t EquipmentSlot(std::uint32_t owner,
                                std::uint32_t found) override {
        Check(owner == player && found == item, "slot lookup arguments");
        calls.emplace_back("slot");
        return slot;
    }
    void RefreshPlayer(std::uint32_t owner) override {
        Check(owner == player, "refresh player");
        calls.emplace_back("refresh-player");
    }
    void UpdateList(pal4::EquipmentWindowState&, std::uint32_t category,
                    std::uint32_t character) override {
        Check(category == 3u && character == 1u, "list arguments");
        calls.emplace_back("list");
    }
    void UpdateDisplay(pal4::EquipmentWindowState&, std::uint32_t found,
                       std::uint32_t character) override {
        Check(found != 0u && character == 1u, "display arguments");
        calls.emplace_back("display");
    }
    std::uint32_t HoveredItem(pal4::EquipmentWindowState&) override {
        calls.emplace_back("hover");
        return hovered.at(hover_index++);
    }
    void RemoveInventoryItem(std::uint32_t found) override {
        Check(found == 0xc000u, "remove first hovered item");
        calls.emplace_back("remove");
    }
    std::string ItemInfoFormat(std::uint32_t id) override {
        Check(id == 0x6au, "GameDB format id");
        calls.emplace_back("format-lookup");
        return "Item: %s";
    }
    std::string ItemText(std::uint32_t address) override {
        Check(address == 0xd000u, "item text pointer");
        calls.emplace_back("item-text");
        return "Sword";
    }
    std::string FormatItemInfo(std::string_view format,
                               std::string_view text) override {
        Check(format == "Item: %s", "format string");
        calls.emplace_back("format");
        return "Item: " + std::string(text);
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "callback allocation size");
        calls.emplace_back("allocate");
        return callback;
    }
    void InitializeDiscardCallback(std::uint32_t value,
                                    std::uint32_t owner) override {
        Check(value == callback && owner == 0x6000u,
              "discard callback captures owner");
        calls.emplace_back("initialize");
    }
    void ShowCommonDialog(std::string_view text, float x, float y,
                          std::uint32_t style,
                          std::uint32_t value) override {
        Check(text == "Item: Sword" && x > 0.299f && x < 0.301f &&
                  y == x && style == 3u && value == callback,
              "common dialog arguments");
        calls.emplace_back("dialog");
        if (throw_dialog) throw std::runtime_error("dialog failure");
    }
    void ReleaseCallback(std::uint32_t value,
                         std::uint32_t flags) override {
        Check(value == callback && flags == 1u, "callback release flags");
        calls.emplace_back("release");
    }
    void ClearItemDetails(pal4::EquipmentWindowState&) override {
        calls.emplace_back("clear");
    }
    void SetEquipmentVisibility(pal4::EquipmentWindowState&,
                                bool visible) override {
        Check(visible, "list refresh shows equipment");
        calls.emplace_back("equipment-visible");
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window >= 0xe000u && window <= 0xe003u,
              "character marker window");
        calls.emplace_back(visible ? "show-marker" : "hide-marker");
    }
    std::uint32_t CharacterLevel(std::uint32_t owner,
                                 std::uint32_t field) override {
        Check(owner == player && field == 4u, "level virtual field");
        calls.emplace_back("level");
        return 27u;
    }
    std::string FormatLevel(std::uint32_t level) override {
        Check(level == 27u, "format level");
        calls.emplace_back("format-level");
        return "Lv. 27";
    }
    void SetText(std::uint32_t window, std::string_view text) override {
        Check(window == 0xb400u && text == "Lv. 27", "level label");
        calls.emplace_back("text");
    }
    void SetSortDirection(std::uint32_t list,
                          std::uint32_t direction) override {
        Check(list == 0x4000u && direction == 0u, "sort direction");
        calls.emplace_back("sort");
    }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x4000u, "list reset target");
        calls.emplace_back("reset");
    }
    void PopulateList(pal4::EquipmentWindowState&,
                      std::uint32_t category) override {
        Check(category == 3u, "populate category");
        calls.emplace_back("populate");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x4000u, "row count target");
        calls.emplace_back("rows");
        return rows;
    }
    void SetHoverRow(std::uint32_t list, std::uint32_t row) override {
        Check(list == 0x4000u && row == 0u, "first row hovered");
        calls.emplace_back("hover-row");
    }
    void SetCharacterImage(pal4::EquipmentWindowState&,
                           std::uint32_t character) override {
        Check(character == 1u || character == 4u,
              "character image index");
        calls.emplace_back("character-image");
    }
    void SetSelected(std::uint32_t button, bool selected) override {
        Check(button >= 0x5000u && button <= 0x5005u,
              "category button");
        if (selected) Check(button == 0x5003u, "selected category");
        calls.emplace_back(selected ? "select" : "unselect");
    }
    void UpdateCharacterEquipment(pal4::EquipmentWindowState&,
                                   std::uint32_t character) override {
        Check(character == 1u || character == 4u,
              "equipment character index");
        calls.emplace_back("character-equipment");
    }
    void UpdateItemDetails(pal4::EquipmentWindowState&,
                           std::uint32_t found) override {
        calls.emplace_back(found == 0u ? "empty-details" : "details");
    }
};

pal4::EquipmentWindowState MakeState() {
    pal4::EquipmentWindowState state{};
    state.Write(0x40u, 0x4000u);
    state.Write(0xb4u, 0xb400u);
    state.Write(0xf0u, 3u);
    state.Write(0xf4u, 1u);
    for (std::uint32_t i = 0; i < 6u; ++i)
        state.Write(0x50u + i * 4u, 0x5000u + i);
    for (std::uint32_t i = 0; i < 4u; ++i)
        state.Write(0xe0u + i * 4u, 0xe000u + i);
    return state;
}

void TestEquip() {
    auto state = MakeState();
    Runtime runtime;
    runtime.words[runtime.item + 0x190u] = 0x777u;
    Check(pal4::EquipmentWindowActions::Equip(state, 0x123u, runtime),
          "equip succeeds");
    Check(runtime.calls == std::vector<std::string>{
              "player", "item", "wa13.wav", "slot", "read", "write",
              "refresh-player", "list", "display"},
          "equip call order");
    runtime = Runtime{};
    runtime.player = 0u;
    Check(!pal4::EquipmentWindowActions::Equip(state, 0x123u, runtime) &&
              runtime.calls == std::vector<std::string>{"player"},
          "equip stops without player");
    runtime = Runtime{};
    runtime.item = 0u;
    Check(!pal4::EquipmentWindowActions::Equip(state, 0x123u, runtime) &&
              runtime.calls == std::vector<std::string>{"player", "item"},
          "equip stops without item");
    runtime = Runtime{};
    runtime.slot = 6u;
    Check(pal4::EquipmentWindowActions::Equip(state, 0x123u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "player", "item", "wa13.wav", "slot",
                  "refresh-player", "list", "display"},
          "out-of-range slot skips write but refreshes");
}

void TestDiscardPrompt() {
    auto state = MakeState();
    Runtime runtime;
    runtime.words[runtime.item + 0x9cu] = 0xd000u;
    Check(pal4::EquipmentWindowActions::ShowDiscardConfirmation(
              state, 0x6000u, 0x123u, runtime), "info succeeds");
    Check(runtime.calls == std::vector<std::string>{
              "player", "item", "read", "item-text", "format-lookup",
              "format", "allocate", "initialize", "dialog", "release"},
          "info formatting, callback and release order");
    runtime.calls.clear();
    runtime.throw_dialog = true;
    bool threw = false;
    try {
        (void)pal4::EquipmentWindowActions::ShowDiscardConfirmation(
            state, 0x6000u, 0x123u, runtime);
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && runtime.calls.back() == "release",
          "callback releases when dialog throws");
    runtime = Runtime{};
    runtime.player = 0u;
    Check(!pal4::EquipmentWindowActions::ShowDiscardConfirmation(
              state, 0x6000u, 0x123u, runtime) &&
              runtime.calls == std::vector<std::string>{"player"},
          "info stops without player");
    runtime = Runtime{};
    runtime.item = 0u;
    Check(!pal4::EquipmentWindowActions::ShowDiscardConfirmation(
              state, 0x6000u, 0x123u, runtime) &&
              runtime.calls == std::vector<std::string>{"player", "item"},
          "info stops without item");
}

void TestDiscard() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::EquipmentWindowActions::ConfirmDiscard(state, 2u, runtime) &&
              runtime.calls.empty(), "answer two cancels without lookup");
    runtime.player = 0u;
    Check(!pal4::EquipmentWindowActions::ConfirmDiscard(state, 1u, runtime) &&
              runtime.calls == std::vector<std::string>{"player"},
          "discard stops without player");
    runtime = Runtime{};
    runtime.hovered = {0u};
    Check(!pal4::EquipmentWindowActions::ConfirmDiscard(state, 1u, runtime) &&
              runtime.calls == std::vector<std::string>{"player", "hover"},
          "discard stops without hovered item");
    runtime = Runtime{};
    runtime.hovered = {0xc000u, 0u};
    Check(!pal4::EquipmentWindowActions::ConfirmDiscard(state, 1u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "player", "hover", "wa07.wav", "remove", "list", "hover"},
          "discard may remove item yet return false for empty list");
    runtime = Runtime{};
    runtime.hovered = {0xc000u, 0xc100u};
    Check(pal4::EquipmentWindowActions::ConfirmDiscard(state, 1u, runtime) &&
              runtime.calls.back() == "display", "discard displays next item");
}

void TestListAndDisplay() {
    auto state = MakeState();
    Runtime runtime;
    pal4::EquipmentWindowActions::UpdateList(state, 3u, 1u, runtime);
    Check(state.Read(0xf0u) == 3u && state.Read(0xf4u) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "clear", "equipment-visible", "hide-marker", "hide-marker",
                  "hide-marker", "hide-marker", "show-marker", "player",
                  "level", "format-level", "text", "sort", "reset",
                  "populate", "rows", "hover-row", "character-image",
                  "unselect", "unselect", "unselect", "unselect",
                  "unselect", "unselect", "select", "character-equipment"},
          "list refresh state and call order");
    runtime = Runtime{};
    runtime.rows = 0u;
    pal4::EquipmentWindowActions::UpdateList(state, 3u, 4u, runtime);
    Check(state.Read(0xf4u) == 4u, "character sentinel stored");
    for (const auto& call : runtime.calls)
        Check(call != "player" && call != "hover-row" &&
                  call != "show-marker", "sentinel and empty rows skip work");
    runtime.calls.clear();
    pal4::EquipmentWindowActions::UpdateDisplay(state, 0u, 1u, runtime);
    Check(runtime.calls == std::vector<std::string>{"empty-details"},
          "empty item updates details alone");
    runtime.calls.clear();
    pal4::EquipmentWindowActions::UpdateDisplay(state, 0xc100u, 1u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "character-equipment", "details"},
          "nonempty item updates equipment before details");
}

}  // namespace

int main() {
    TestEquip();
    TestDiscardPrompt();
    TestDiscard();
    TestListAndDisplay();
    std::cout << "Equipment window action tests passed\n";
}
