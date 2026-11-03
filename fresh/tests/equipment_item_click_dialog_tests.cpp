#include "pal4/equipment_item_click_dialog.h"

#include <algorithm>
#include <array>
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

struct Button {
    std::uint32_t item_id;
    std::string action;
    std::string text;
    bool enabled;
};

struct Runtime final : pal4::EquipmentItemClickRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::array<std::uint32_t, 4> players{0xa000u, 0xb000u, 0u, 0u};
    std::vector<std::string> calls;
    std::vector<Button> buttons;
    std::uint32_t item{0x8000u};
    std::int32_t level{10};
    bool owned{};
    bool fail_conversion{};

    std::uint32_t HoveredItem(pal4::EquipmentWindowState&) override {
        calls.emplace_back("hover");
        return item;
    }
    std::uint32_t Dialog() override {
        calls.emplace_back("dialog");
        return 0x7000u;
    }
    void HideAndCleanup(std::uint32_t dialog) override {
        Check(dialog == 0x7000u, "hide selected dialog");
        calls.emplace_back("hide");
    }
    std::uint32_t Player(std::uint32_t index) override {
        Check(index < 4u, "player index in 0-3");
        calls.emplace_back("player:" + std::to_string(index));
        return players[index];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    bool ContainsEquipment(std::uint32_t player,
                            std::uint32_t item_id) override {
        Check(player != 0u && item_id == 0x55u,
              "ownership lookup uses player and item instance id");
        calls.emplace_back("contains:" + std::to_string(player));
        return owned && player == 0xa000u;
    }
    std::int32_t CharacterLevel(std::uint32_t player,
                                std::uint32_t field) override {
        Check(player == players[1] && field == 4u,
              "selected player level field four");
        calls.emplace_back("level");
        return level;
    }
    std::string TriggerText(std::uint32_t trigger_id) override {
        Check(trigger_id == 0x18u || trigger_id == 0x17u,
              "GameDB trigger index");
        calls.emplace_back("trigger:" + std::to_string(trigger_id));
        return trigger_id == 0x18u ? "Use" : "Discard";
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("utf8:" + std::string(text));
        if (fail_conversion) throw std::runtime_error("conversion failed");
        return "utf8:" + std::string(text);
    }
    void CreateButton(std::uint32_t dialog,
                      pal4::EquipmentDialogButton button) override {
        Check(dialog == 0x7000u, "button dialog");
        calls.emplace_back("button:" + std::string(button.action));
        buttons.push_back({button.item_id, std::string(button.action),
                           std::string(button.text), button.enabled});
    }
    void ShowDialog(std::uint32_t dialog,
                    std::uint32_t event_data) override {
        Check(dialog == 0x7000u && event_data == 0x500cu,
              "dialog receives forwarded event payload");
        calls.emplace_back("show");
    }
    void DestroyCeguiString(std::uint32_t address) override {
        calls.emplace_back("destroy:" + std::to_string(address));
    }
};

pal4::EquipmentWindowState MakeState(std::uint32_t character = 1u) {
    pal4::EquipmentWindowState state{};
    state.Write(0xf4u, character);
    return state;
}

Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[0x8190u] = 0x55u;
    runtime.words[0x816cu] = 5u;
    runtime.words[0xa000u + 0xb08u] = 1u;
    runtime.words[0xb000u + 0xb08u] = 1u;
    return runtime;
}

void TestEarlyExits() {
    auto state = MakeState(4u);
    auto runtime = MakeRuntime();
    Check(pal4::EquipmentItemClickDialog::Open(state, 0x500cu, runtime),
          "sentinel returns true");
    Check(runtime.calls.empty(), "sentinel never queries hovered item");

    state = MakeState();
    runtime.item = 0u;
    Check(pal4::EquipmentItemClickDialog::Open(state, 0x500cu, runtime),
          "empty hover returns true");
    Check(runtime.calls == std::vector<std::string>{"hover"},
          "empty hover does not touch dialog");
}

void TestAvailableItem() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    Check(pal4::EquipmentItemClickDialog::Open(state, 0x500cu, runtime),
          "available item returns true");
    Check(runtime.buttons.size() == 2u, "creates two choices");
    Check(runtime.buttons[0].item_id == 0x55u &&
              runtime.buttons[0].action == "use_equipment" &&
              runtime.buttons[0].text == "utf8:Use" &&
              runtime.buttons[0].enabled,
          "available item enables use choice with trigger 0x18 text");
    Check(runtime.buttons[1].item_id == 0x55u &&
              runtime.buttons[1].action == "discard_equipment" &&
              runtime.buttons[1].text == "utf8:Discard" &&
              runtime.buttons[1].enabled,
          "available item enables discard with trigger 0x17 text");
    Check(runtime.calls[0] == "hover" &&
              runtime.calls[1] == "dialog" &&
              runtime.calls[2] == "hide" &&
              runtime.calls.back() == "show",
          "dialog is hidden before checks and shown after buttons");
    const auto use_button = std::find(runtime.calls.begin(),
                                       runtime.calls.end(),
                                       "button:use_equipment");
    const auto discard_button = std::find(runtime.calls.begin(),
                                           runtime.calls.end(),
                                           "button:discard_equipment");
    Check(use_button < discard_button &&
              discard_button < runtime.calls.end() - 1,
          "button creation order");
}

void TestOwnershipAndLevelGates() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    runtime.owned = true;
    Check(pal4::EquipmentItemClickDialog::Open(state, 0x500cu, runtime),
          "owned item returns true");
    Check(!runtime.buttons[0].enabled && !runtime.buttons[1].enabled,
          "active player's ownership disables both choices");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "player:2") == runtime.calls.end(),
          "ownership search stops at first match");

    runtime = MakeRuntime();
    runtime.owned = true;
    runtime.words[0xa000u + 0xb08u] = 0u;
    Check(pal4::EquipmentItemClickDialog::Open(state, 0x500cu, runtime),
          "inactive owner's item returns true");
    Check(runtime.buttons[0].enabled && runtime.buttons[1].enabled,
          "inactive owner does not block choices");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "contains:40960") == runtime.calls.end(),
          "inactive player bypasses inventory contains");

    runtime = MakeRuntime();
    runtime.level = 4;
    Check(pal4::EquipmentItemClickDialog::Open(state, 0x500cu, runtime),
          "low-level item returns true");
    Check(!runtime.buttons[0].enabled && runtime.buttons[1].enabled,
          "insufficient level blocks use but allows discard");

    runtime = MakeRuntime();
    runtime.words[0x816cu] = 0u;
    Check(pal4::EquipmentItemClickDialog::Open(state, 0x500cu, runtime),
          "zero requirement item returns true");
    Check(!runtime.buttons[0].enabled && runtime.buttons[1].enabled,
          "nonpositive requirement blocks use");
}

void TestConversionFailure() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    runtime.fail_conversion = true;
    bool threw = false;
    try {
        (void)pal4::EquipmentItemClickDialog::Open(state, 0x500cu, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.buttons.empty() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "show") == runtime.calls.end(),
          "conversion failure does not create or show choices");
}

void TestCleanupStringThunk() {
    Runtime runtime;
    pal4::EquipmentItemClickDialog::CleanupString(0x5000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"destroy:20492"},
          "cleanup thunk destroys CEGUI string at object +0xc");
}

}  // namespace

int main() {
    TestEarlyExits();
    TestAvailableItem();
    TestOwnershipAndLevelGates();
    TestConversionFailure();
    TestCleanupStringThunk();
}
