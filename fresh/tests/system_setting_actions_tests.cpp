#include "pal4/system_setting_actions.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingActionsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, bool>> selected;
    std::vector<std::uint32_t> redrawn;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::tuple<std::uint32_t, std::string, std::string> image;
    std::uint32_t focused_member = 0u;
    std::uint32_t focused_button = 0u;
    std::uint32_t visible_window = 0u;
    bool data_mode = false;
    bool null_callback = false;
    bool fail_dialog = false;
    std::uint32_t shown_callback = 0u;
    std::uint32_t released_callback = 0u;
    std::uint32_t arena_resets = 0u;
    std::uint32_t ui_events = 0u;
    std::uint32_t game_initializations = 0u;

    Runtime() {
        words[0x4000u + 188u] = 0x5000u;
        words[0x4000u + 184u] = 0x5001u;
        words[0x4000u + 192u] = 0x5002u;
        words[0x4000u + 196u] = 0x5003u;
        words[0x4000u + 180u] = 0x6000u;
        words[0x4000u + 48u] = 0x7000u;
        words[0x4000u + 52u] = 0x700cu;
        words[0x7000u] = 0x8000u;
        words[0x7004u] = 0x8001u;
        words[0x7008u] = 0x8002u;
        for (std::uint32_t i = 0; i < 10u; ++i) {
            words[0x4000u + 200u + 4u * i] = 0x9000u + i;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("byte");
        bytes[address] = value;
    }
    void ShowDataMode(std::uint32_t frame) override {
        calls.emplace_back("data");
        Check(frame == 0x4000u &&
              (words[0x41f8u] == 1u || words[0x41f8u] == 2u),
              "mode is set before data page");
        data_mode = true;
    }
    void ShowSave(std::uint32_t frame) override {
        calls.emplace_back("save");
        Check(frame == 0x4000u && words[0x41f8u] == 0u,
              "system mode set before save view");
    }
    void SetFocusedButton(std::uint32_t member,
                          std::uint32_t button) override {
        calls.emplace_back("focus");
        focused_member = member;
        focused_button = button;
    }
    void SetSelected(std::uint32_t button, bool value) override {
        calls.emplace_back("selected");
        selected.emplace_back(button, value);
    }
    void RequestRedraw(std::uint32_t window) override {
        calls.emplace_back("redraw");
        redrawn.push_back(window);
    }
    void SetVisible(std::uint32_t window, bool value) override {
        calls.emplace_back("visible");
        Check(value, "load image visible");
        visible_window = window;
    }
    void SetImage(std::uint32_t window, std::string_view name,
                  std::string_view imageset) override {
        calls.emplace_back("image");
        image = {window, std::string(name), std::string(imageset)};
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == 0x18u, "quit callback allocation size");
        return null_callback ? 0u : 0xc000u;
    }
    std::uint32_t TriggerText(std::uint32_t trigger_id) override {
        calls.emplace_back("trigger");
        Check(trigger_id == 217u, "quit confirmation trigger");
        return 0xd000u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0xe000u;
    }
    void CreateCommonDialog(std::uint32_t manager) override {
        calls.emplace_back("create-dialog");
        Check(manager == 0xe000u, "confirmation dialog manager");
    }
    std::uint32_t ShowDialog(std::uint32_t text,
                             std::uint32_t x_bits,
                             std::uint32_t y_bits,
                             std::uint32_t mode,
                             std::uint32_t callback) override {
        calls.emplace_back("show-dialog");
        Check(text == 0xd000u && x_bits == 1050253722u &&
              y_bits == 1053609165u && mode == 3u,
              "confirmation dialog content, size and mode");
        shown_callback = callback;
        if (fail_dialog) throw std::runtime_error("dialog");
        return 0x77u;
    }
    std::uint32_t ReleaseCallback(std::uint32_t callback) override {
        calls.emplace_back("release");
        released_callback = callback;
        return 0x88u;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0xf000u;
    }
    void ResetArena(std::uint32_t arena) override {
        calls.emplace_back("reset-arena");
        Check(arena == 0xf000u, "arena reset target");
        ++arena_resets;
    }
    void ProcessUiEvent(std::uint32_t dispatcher,
                        std::uint32_t event,
                        std::uint32_t first,
                        std::uint32_t second) override {
        calls.emplace_back("ui-event");
        Check(dispatcher == 0xe000u + 260u && event == 0x8a80b4u &&
              first == 0u && second == 0u,
              "quit UI event payload");
        ++ui_events;
    }
    std::uint32_t Game() override {
        calls.emplace_back("game");
        return 0x11000u;
    }
    void InitializeGameState(std::uint32_t game,
                             std::uint32_t first,
                             std::uint32_t second) override {
        calls.emplace_back("initialize-game");
        Check(game == 0x11000u && first == 0u && second == 0u,
              "game state initialization");
        ++game_initializations;
    }
};

void CheckResult(const Runtime& runtime) {
    Check(runtime.data_mode && runtime.words.at(0x41f8u) == 2u &&
          runtime.words.at(0x403cu) == 1u &&
          runtime.focused_member == 0x4028u &&
          runtime.focused_button == 0x5000u &&
          runtime.visible_window == 0x6000u &&
          runtime.image == std::tuple{0x6000u,
                                      std::string("zi0"),
                                      std::string("duquyouxi")},
          "load-data state and image");
}

}  // namespace

int main() {
    Runtime event;
    Check(pal4::SystemSettingActions::OnLoadData(
          0x4000u, 0xdeadbeefu, event) == 1u,
          "callback ignores event and returns true");
    CheckResult(event);
    Check(event.selected == std::vector<std::pair<std::uint32_t, bool>>{
          {0x8000u, false}, {0x8001u, false}, {0x8002u, false},
          {0x5000u, true}}, "clear active buttons before load selection");
    Check(event.calls == std::vector<std::string>{
          "write", "data", "read", "focus", "read", "read",
          "read", "selected", "read", "selected", "read", "selected",
          "selected", "read", "visible", "image", "write"},
          "load-data call order");

    Runtime empty;
    empty.words[0x4034u] = 0x7000u;
    pal4::SystemSettingActions::SelectLoadData(0x4000u, empty);
    CheckResult(empty);
    Check(empty.selected == std::vector<std::pair<std::uint32_t, bool>>{
          {0x5000u, true}}, "empty active button collection");

    Runtime start;
    Check(pal4::SystemSettingActions::OnStartNewGame(
          0x4000u, 0xabcdefu, start) == 1u &&
          start.words.at(0x41f8u) == 1u &&
          start.words.at(0x403cu) == 0u &&
          start.focused_button == 0x5001u &&
          start.image == std::tuple{0x6000u,
                                    std::string("zi0"),
                                    std::string("chucunyouxi")},
          "start-new-game state and image");
    Check(start.selected == std::vector<std::pair<std::uint32_t, bool>>{
          {0x8000u, false}, {0x8001u, false}, {0x8002u, false},
          {0x5001u, true}}, "start-new-game main button selection");
    Check(start.redrawn.size() == 10u, "all setting buttons redrawn");
    for (std::uint32_t i = 0; i < 10u; ++i) {
        const auto control = 0x9000u + i;
        Check(start.redrawn[i] == control &&
              start.bytes.at(control + 1201u) == 0u,
              "setting button state byte cleared before redraw");
    }
    Check(start.calls[0] == "write" && start.calls[1] == "data" &&
          start.calls[2] == "read" && start.calls[3] == "focus" &&
          start.calls.back() == "write" &&
          start.calls[start.calls.size() - 2u] == "image" &&
          start.calls[start.calls.size() - 3u] == "visible",
          "start-new-game call boundaries");

    Runtime system;
    Check(pal4::SystemSettingActions::OnSystemSettings(
          0x4000u, 0x7777u, system) == 1u &&
          system.words.at(0x41f8u) == 0u &&
          system.words.at(0x403cu) == 2u &&
          system.focused_button == 0x5002u &&
          system.image == std::tuple{0x6000u,
                                     std::string("zi0"),
                                     std::string("xitongsheding")},
          "system settings callback state");
    Check(system.selected == std::vector<std::pair<std::uint32_t, bool>>{
          {0x8000u, false}, {0x8001u, false}, {0x8002u, false},
          {0x5002u, true}} &&
          system.redrawn == std::vector<std::uint32_t>{
              0x8000u, 0x8001u, 0x8002u},
          "system settings resets active collection visuals");
    for (std::uint32_t i = 0; i < 3u; ++i) {
        Check(system.bytes.at(0x8000u + i + 1201u) == 0u,
              "system settings clears active state byte");
    }
    Check(system.calls[0] == "write" && system.calls[1] == "save" &&
          system.calls[2] == "read" && system.calls[3] == "focus" &&
          system.calls.back() == "write" &&
          system.calls[6] == "read" && system.calls[7] == "selected" &&
          system.calls[8] == "byte" && system.calls[9] == "redraw",
          "system settings call order");

    Runtime quit;
    Check(pal4::SystemSettingActions::ConfirmQuit(0x4000u, quit) == 0x88u &&
          quit.words.at(0x403cu) == 3u &&
          quit.focused_button == 0x5003u &&
          quit.shown_callback == 0xc000u &&
          quit.released_callback == 0xc000u &&
          quit.words.at(0xc000u) == 0x843c08u &&
          quit.words.at(0xc008u) == 0x481630u &&
          quit.words.at(0xc00cu) == 0u &&
          quit.words.at(0xc010u) == 0x4000u,
          "quit dialog callback layout and return");
    Check(quit.selected == std::vector<std::pair<std::uint32_t, bool>>{
          {0x8000u, false}, {0x8001u, false}, {0x8002u, false},
          {0x5003u, true}}, "quit selected button");
    Check(quit.calls[quit.calls.size() - 5u] == "trigger" &&
          quit.calls[quit.calls.size() - 4u] == "manager" &&
          quit.calls[quit.calls.size() - 3u] == "create-dialog" &&
          quit.calls[quit.calls.size() - 2u] == "show-dialog" &&
          quit.calls.back() == "release", "quit dialog call order");

    Runtime null_callback;
    null_callback.null_callback = true;
    Check(pal4::SystemSettingActions::ConfirmQuit(
          0x4000u, null_callback) == 0x77u &&
          null_callback.shown_callback == 0u &&
          null_callback.released_callback == 0u,
          "null callback returns dialog result");

    Runtime failed;
    failed.fail_dialog = true;
    bool caught = false;
    try {
        (void)pal4::SystemSettingActions::ConfirmQuit(0x4000u, failed);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed.released_callback == 0xc000u,
          "callback released after dialog exception");

    Runtime confirm;
    Check(pal4::SystemSettingActions::OnQuitConfirmed(
          0x4000u, 1u, confirm) == 1u &&
          confirm.arena_resets == 1u && confirm.ui_events == 1u &&
          confirm.game_initializations == 1u &&
          confirm.calls == std::vector<std::string>{
              "arena", "reset-arena", "manager", "ui-event",
              "game", "initialize-game"},
          "confirmed quit dispatches game transition in order");
    Runtime cancel;
    Check(pal4::SystemSettingActions::OnQuitConfirmed(
          0x4000u, 2u, cancel) == 1u &&
          cancel.arena_resets == 0u && cancel.ui_events == 0u &&
          cancel.game_initializations == 0u &&
          cancel.focused_member == 0x4028u &&
          cancel.focused_button == 0x5003u &&
          cancel.selected ==
              std::vector<std::pair<std::uint32_t, bool>>{{0x5003u, false}},
          "cancelled quit restores focus and selection");

    Runtime wrapper;
    Check(pal4::SystemSettingActions::OnQuitGame(
          0x4000u, 0xabcdefu, wrapper) == 1u &&
          wrapper.shown_callback == 0xc000u,
          "quit wrapper ignores event and returns true");
}
