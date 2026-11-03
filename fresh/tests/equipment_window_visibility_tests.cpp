#include "pal4/equipment_window_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentWindowVisibilityRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::string> commands;
    std::vector<std::uint32_t> selected_windows;
    std::vector<std::uint32_t> cleared_images;
    std::vector<std::uint32_t> next_indices;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t next_pos{};

    void SetParent(pal4::EquipmentWindowState&,
                   std::uint8_t showing,
                   std::uint32_t parent) override {
        Check((showing == 0u || showing == 1u) && parent == 0x7000u,
              "visibility forwards parent arguments");
        calls.emplace_back("parent");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    void ProcessEvent(std::uint32_t tree,
                      std::string_view command,
                      std::uint32_t first,
                      std::uint32_t second) override {
        Check(tree == 0x8104u, "command tree at manager +0x104");
        if (command == "show_frame_tool")
            Check(second == 0u && first <= 1u,
                  "frame tool receives visibility and zero");
        else if (command == "show_game_info")
            Check(second == 0x7500u && first <= 1u,
                  "game info receives root window");
        else
            Check(command == "set_help_tips_id" && first == 7u &&
                      second == 0u, "show selects help tip seven");
        commands.emplace_back(command);
        calls.emplace_back("event:" + std::string(command));
    }
    void SetEquipmentVisibility(pal4::EquipmentWindowState&,
                                bool visible) override {
        Check(!visible, "equipment panel hidden before reset");
        calls.emplace_back("equipment");
    }
    void ClearItemDetails(pal4::EquipmentWindowState&) override {
        calls.emplace_back("details");
    }
    void SetSelected(std::uint32_t window,
                     bool selected) override {
        Check(!selected, "category selection cleared");
        selected_windows.push_back(window);
        calls.emplace_back("unselect");
    }
    void SetText(std::uint32_t window,
                 std::u32string_view text) override {
        Check(text.empty(), "equipment image text cleared");
        cleared_images.push_back(window);
        calls.emplace_back("clear-text");
    }
    void SetEnabled(std::uint32_t window,
                    bool enabled) override {
        Check(!enabled && cleared_images.back() == window,
              "equipment image disabled after text clear");
        calls.emplace_back("disable");
    }
    std::uint32_t NextCharacterIndex(std::uint32_t current) override {
        Check(next_pos < next_indices.size() &&
                  current == (next_pos == 0u ? 4u : next_indices[next_pos - 1u]),
              "character search advances from prior index");
        calls.emplace_back("next");
        return next_indices[next_pos++];
    }
    std::uint32_t PlayerByIndex(std::uint32_t index) override {
        calls.emplace_back("player");
        return index == 1u ? 0x9000u : index == 2u ? 0xa000u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-player");
        return words.at(address);
    }
    void InsertActive(std::uint32_t array,
                      std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x6028u,
              "show inserts primary member");
        calls.emplace_back("insert");
    }
    void RemoveActive(std::uint32_t array,
                      std::uint32_t member) override {
        Check(array == 0x8134u && member == 0x6028u,
              "hide removes primary member");
        calls.emplace_back("remove");
    }
    void SelectCharacter(pal4::EquipmentWindowState&,
                         std::uint32_t value) override {
        Check(value == 5u, "show invokes selection with five");
        calls.emplace_back("select-character");
    }
    void UpdateSystemDialogVisibility(std::uint32_t manager,
                                      std::uint32_t dialog) override {
        Check(manager == 0x8000u && dialog == 3u,
              "equipment dialog updates system slot three");
        calls.emplace_back("system-dialog");
    }
    void ToggleModelVisibility(pal4::EquipmentWindowState&,
                               bool visible) override {
        Check(!visible, "hide turns off model");
        calls.emplace_back("hide-model");
    }
    std::uint32_t RefreshMember(std::uint32_t member) override {
        Check(member == 0x6118u, "secondary member refreshed last");
        calls.emplace_back("refresh");
        return 0xbeefu;
    }
};

pal4::EquipmentWindowState MakeState() {
    pal4::EquipmentWindowState state{};
    state.Write(0x24u, 0x7500u);
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        state.Write(0x50u + index * 4u, 0x2000u + index * 0x100u);
        state.Write(0x98u + index * 4u, 0x3000u + index * 0x100u);
    }
    return state;
}

void CheckShow() {
    auto state = MakeState();
    Runtime runtime;
    runtime.next_indices = {1u, 2u};
    runtime.words = {{0x9000u + 0xb08u, 0u},
                     {0xa000u + 0xb08u, 1u}};
    Check(pal4::EquipmentWindowVisibility::SetVisible(
              state, 0x6000u, 1u, 0x7000u, runtime) == 0xbeefu &&
                  state.Read(0xf4u) == 2u &&
                  runtime.next_pos == 2u &&
                  runtime.commands == std::vector<std::string>{
                      "show_frame_tool", "show_game_info",
                      "set_help_tips_id"},
          "show searches until active player and sends three commands");
    Check(runtime.selected_windows == std::vector<std::uint32_t>{
              0x2000u, 0x2100u, 0x2200u, 0x2300u, 0x2400u, 0x2500u} &&
                  runtime.cleared_images == std::vector<std::uint32_t>{
                      0x3000u, 0x3100u, 0x3200u, 0x3300u,
                      0x3400u, 0x3500u},
          "six category buttons and six equipment images reset");
    const auto find = [&](std::string_view value) {
        for (std::size_t i = 0u; i < runtime.calls.size(); ++i)
            if (runtime.calls[i] == value) return i;
        return runtime.calls.size();
    };
    Check(find("parent") < find("event:show_frame_tool") &&
                  find("event:show_game_info") < find("equipment") &&
                  find("details") < find("unselect") &&
                  find("clear-text") < find("next") &&
                  find("event:set_help_tips_id") < find("insert") &&
                  find("insert") < find("select-character") &&
                  find("system-dialog") < find("refresh"),
          "show branch preserves IDA call ordering");
}

void CheckHideAndSentinel() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::EquipmentWindowVisibility::SetVisible(
              state, 0x6000u, 0u, 0x7000u, runtime) == 0xbeefu &&
                  runtime.commands == std::vector<std::string>{
                      "show_frame_tool", "show_game_info"} &&
                  runtime.next_pos == 0u &&
                  runtime.calls[runtime.calls.size() - 3u] == "manager" &&
                  runtime.calls[runtime.calls.size() - 2u] == "remove" &&
                  runtime.calls.back() == "refresh",
          "hide removes member and refreshes secondary member");
    state = MakeState();
    runtime = Runtime{};
    runtime.next_indices = {4u};
    Check(pal4::EquipmentWindowVisibility::SetVisible(
              state, 0x6000u, 1u, 0x7000u, runtime) == 0xbeefu &&
                  state.Read(0xf4u) == 4u &&
                  runtime.next_pos == 1u,
          "sentinel four ends character search without player lookup");
    for (const auto& call : runtime.calls)
        Check(call != "player", "sentinel path skips player lookup");
}

}  // namespace

int main() {
    CheckShow();
    CheckHideAndSentinel();
    std::cout << "Equipment window visibility tests passed\n";
}
