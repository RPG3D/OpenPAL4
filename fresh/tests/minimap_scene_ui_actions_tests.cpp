#include "pal4/minimap_scene_ui_actions.h"

#include <cstdlib>
#include <iostream>
#include <limits>
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

struct Runtime final : pal4::IMinimapSceneUiHelpRuntime {
    std::vector<std::string> calls;
    std::uint32_t state = 0u;
    std::uint32_t key_cell = 0u;
    pal4::MinimapSceneUiEventData event{};
    pal4::MinimapSceneUiAttributeLookup lookup{0x1000u, 0x1000u};
    std::uint32_t frame_written = 0u;
    std::uint32_t minimap_write = 0xffffffffu;
    bool window_visible = false;
    bool dialog_visible = true;
    const pal4::MinimapSceneUiState* input_state = nullptr;
    bool throw_lookup = false;
    bool throw_add = false;
    std::int32_t run_mode = 0;
    std::int32_t next_run_mode = -1;
    int actor_control_calls = 0;

    std::uint32_t Game() override {
        calls.emplace_back("game");
        return 0x5000u;
    }
    void InitializeStateAndUi(std::uint32_t game,
                              std::uint32_t new_state,
                              std::uint32_t payload) override {
        Check(game == 0x5000u &&
                  ((new_state == 2u && payload == 0u) ||
                   (new_state == 10u && payload == 0x8000u)),
              "system state game and payload");
        state = new_state;
        calls.emplace_back("initialize");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    void ProcessEvent(std::uint32_t dispatcher,
                      std::uint32_t key,
                      pal4::MinimapSceneUiEventData data) override {
        Check(dispatcher == 0x7104u, "UI event dispatcher offset");
        key_cell = key;
        event = data;
        calls.emplace_back("event");
    }
    void PlayMusic(std::string_view file,
                   std::uint32_t mode) override {
        Check(file == "WA22.wav" && mode == 1u,
              "toggle music file and mode");
        calls.emplace_back("music");
    }
    void SetWindowVisible(std::uint32_t window,
                          bool visible) override {
        Check(window == 0x6000u, "minimap visibility window");
        window_visible = visible;
        calls.emplace_back("window");
    }
    bool DialogIsVisible(std::uint32_t owner) override {
        Check(owner == 0x5000u, "minimap dialog owner");
        calls.emplace_back("dialog-visible");
        return dialog_visible;
    }
    std::uint32_t MinimapManager() override {
        calls.emplace_back("minimap-manager");
        return 0x9000u;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x9054u, "minimap manager flag offset");
        minimap_write = value;
        calls.emplace_back("write-byte");
    }
    std::uint32_t ActorControl() override {
        ++actor_control_calls;
        calls.emplace_back("actor-control");
        return 0xb000u;
    }
    std::int32_t ActorControlMode(std::uint32_t control) override {
        Check(control == 0xb000u, "run mode control pointer");
        calls.emplace_back("run-mode");
        return run_mode;
    }
    void SetActorControlMode(std::uint32_t control,
                             std::uint32_t context,
                             std::int32_t mode) override {
        Check(control == 0xb000u && context == 0xc000u,
              "run mode setter control and EBP context");
        next_run_mode = mode;
        calls.emplace_back("set-run-mode");
    }
    void UpdateCharacterMovementState(
        std::uint32_t owner) override {
        Check(owner == 0x5000u, "movement refresh owner");
        calls.emplace_back("movement");
    }
    std::uint32_t InputManager() override {
        calls.emplace_back("input-manager");
        return 0xd000u;
    }
    void ResetInputState(std::uint32_t manager) override {
        Check(manager == 0xd000u, "input manager reset target");
        Check(input_state != nullptr &&
                  input_state->ReadByte(0xf6u) == 0u,
              "scene flag clears before input reset");
        calls.emplace_back("input-reset");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x7144u, "help reads manager state flag");
        calls.emplace_back("read");
        return 7u;
    }
    std::uint32_t InitializeDocument() override {
        calls.emplace_back("document-init");
        return 0x8000u;
    }
    void DestroyDocument(std::uint32_t document) override {
        Check(document == 0x8000u, "XML document cleanup handle");
        calls.emplace_back("document-destroy");
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::uint32_t key) override {
        Check(manager == 0x7000u && key == 0x8b13acu,
              "help frame lookup key");
        calls.emplace_back("frame");
        return 0x9000u;
    }
    void BeginFrameName(std::string_view name) override {
        Check(name == "frame", "frame attribute name");
        calls.emplace_back("name-begin");
    }
    void EndFrameName() override {
        calls.emplace_back("name-end");
    }
    pal4::MinimapSceneUiAttributeLookup FindAttribute(
        std::uint32_t document, std::string_view name) override {
        Check(document == 0x8000u && name == "frame",
              "XML attribute lookup");
        calls.emplace_back("lookup");
        if (throw_lookup) throw std::runtime_error("lookup");
        return lookup;
    }
    std::uint32_t CreateFrameAttribute(std::string_view name,
                                       std::uint32_t frame) override {
        Check(name == "frame" && frame == 0x9000u,
              "XML attribute creation");
        calls.emplace_back("create");
        return 0xa000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == 0x8000u && attribute == 0xa000u,
              "XML attribute append");
        calls.emplace_back("add");
        if (throw_add) throw std::runtime_error("add");
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        Check(attribute == 0xa000u, "XML temporary attribute cleanup");
        calls.emplace_back("attribute-destroy");
    }
    void SetExistingAttribute(std::uint32_t attribute_value,
                              std::uint32_t frame) override {
        Check(attribute_value == lookup.position + 0x1cu,
              "XML existing attribute value offset");
        frame_written = frame;
        calls.emplace_back("set-attribute");
    }
};

std::size_t Position(const Runtime& runtime, const std::string& call) {
    for (std::size_t index = 0; index < runtime.calls.size(); ++index) {
        if (runtime.calls[index] == call) return index;
    }
    return runtime.calls.size();
}

}  // namespace

int main() {
    Runtime helper;
    Check(pal4::MinimapSceneUiActions::SwitchToSystem(helper) == 1u &&
              helper.state == 2u &&
              helper.calls == std::vector<std::string>{"game", "initialize"},
          "state helper initializes system state and returns true");

    Runtime click;
    Check(pal4::MinimapSceneUiActions::SystemClick(
              0xffffffffu, click) == 1u &&
              click.state == 2u &&
              click.key_cell == 0x8a80b8u &&
              click.event.kind == 1u && click.event.value == 0u &&
              click.calls == std::vector<std::string>{
                                 "game", "initialize", "manager", "event"},
          "system click composes state switch before event dispatch");

    Runtime missing;
    Check(pal4::MinimapSceneUiActions::HelpClick(
              0xffffffffu, missing) == 1u &&
              missing.key_cell == 0x8b1c30u &&
              missing.event.kind == 1u && missing.event.value == 7u &&
              missing.state == 10u &&
              Position(missing, "event") < Position(missing, "document-init") &&
              Position(missing, "name-end") < Position(missing, "create") &&
              Position(missing, "attribute-destroy") <
                  Position(missing, "initialize") &&
              missing.calls.back() == "document-destroy",
          "help click creates missing frame attribute before state change");

    Runtime existing;
    existing.lookup = {0x2000u, 0x3000u};
    Check(pal4::MinimapSceneUiActions::HelpClick(
              0u, existing) == 1u &&
              existing.frame_written == 0x9000u &&
              Position(existing, "create") == existing.calls.size() &&
              Position(existing, "set-attribute") <
                  Position(existing, "initialize"),
          "existing frame attribute is updated in place");

    Runtime sentinel;
    sentinel.lookup = {0xffffffe4u, 0x3000u};
    (void)pal4::MinimapSceneUiActions::HelpClick(0u, sentinel);
    Check(Position(sentinel, "create") < sentinel.calls.size(),
          "minus-28 cursor uses missing-attribute branch");

    Runtime failed_lookup;
    failed_lookup.throw_lookup = true;
    bool caught = false;
    try {
        (void)pal4::MinimapSceneUiActions::HelpClick(0u, failed_lookup);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed_lookup.calls[failed_lookup.calls.size() - 2u] ==
                        "name-end" &&
              failed_lookup.calls.back() == "document-destroy",
          "lookup failure releases name and XML document");

    Runtime failed_add;
    failed_add.throw_add = true;
    caught = false;
    try {
        (void)pal4::MinimapSceneUiActions::HelpClick(0u, failed_add);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed_add.calls[failed_add.calls.size() - 2u] ==
                        "attribute-destroy" &&
              failed_add.calls.back() == "document-destroy",
          "append failure releases attribute and XML document");

    pal4::MinimapSceneUiState toggle_state;
    toggle_state.WriteWord(0x54u, 0x6000u);
    Runtime toggle;
    Check(pal4::MinimapSceneUiActions::ToggleState(
              toggle_state, 0x5000u, 0x1234u, toggle) == 1u &&
              toggle_state.ReadByte(0xf5u) == 1u &&
              toggle.window_visible && toggle.minimap_write == 1u &&
              toggle.calls == std::vector<std::string>{
                                  "music", "window", "dialog-visible",
                                  "minimap-manager", "write-byte"},
          "zero state byte enables state and sound update");
    Check(pal4::MinimapSceneUiActions::ToggleState(
              toggle_state, 0x5000u, 0u, toggle) == 1u &&
              toggle_state.ReadByte(0xf5u) == 0u &&
              !toggle.window_visible && toggle.minimap_write == 0u &&
              toggle.calls.size() == 9u,
          "nonzero state byte disables state and sound update");
    Check(pal4::MinimapSceneUiActions::SetStateAndPlaySound(
              toggle_state, 0x5000u, 0x80u, toggle) == 0x9000u &&
              toggle_state.ReadByte(0xf5u) == 0x80u &&
              toggle.calls[toggle.calls.size() - 4u] == "music" &&
              toggle.calls.back() == "write-byte",
          "state helper stores raw byte, plays music, returns refresh result");
    Runtime flag;
    Check(pal4::MinimapSceneUiActions::SetFlagAndUpdate(
              toggle_state, 0x5000u, 0x80u, flag) == 0x9000u &&
              toggle_state.ReadByte(0xf4u) == 0x80u &&
              flag.calls == std::vector<std::string>{
                                "window", "minimap-manager", "write-byte"},
          "secondary flag stores raw byte then returns refresh result");

    pal4::MinimapSceneUiState display_state;
    display_state.WriteWord(0x54u, 0x6000u);
    display_state.WriteByte(0xf5u, 3u);
    Runtime display;
    Check(pal4::MinimapSceneUiActions::UpdateVisibilityAndMinimapState(
              display_state, 0x5000u, display) == 0x9000u &&
              display.window_visible && display.minimap_write == 1u &&
              display.calls == std::vector<std::string>{
                                   "window", "dialog-visible",
                                   "minimap-manager", "write-byte"},
          "odd state byte and visible dialog enable minimap manager");
    display.dialog_visible = false;
    (void)pal4::MinimapSceneUiActions::UpdateVisibilityAndMinimapState(
        display_state, 0x5000u, display);
    Check(display.window_visible && display.minimap_write == 0u,
          "hidden dialog disables minimap while leaving window enabled");
    display_state.WriteByte(0xf4u, 1u);
    const auto before = display.calls.size();
    (void)pal4::MinimapSceneUiActions::UpdateVisibilityAndMinimapState(
        display_state, 0x5000u, display);
    Check(!display.window_visible && display.minimap_write == 0u &&
              display.calls.size() == before + 3u &&
              display.calls[before] == "window" &&
              display.calls[before + 1u] == "minimap-manager",
          "secondary flag suppresses dialog visibility query");

    for (auto mode : {-2, 0, 1, 2, 3}) {
        Runtime run;
        run.run_mode = mode;
        Check(pal4::MinimapSceneUiActions::RunClick(
                  0x5000u, 0xc000u, 0xffffffffu, run) == 1u &&
                  run.next_run_mode == (mode + 1 >= 3 ? 0 : mode + 1) &&
                  run.actor_control_calls == 2 &&
                  run.calls == std::vector<std::string>{
                                   "actor-control", "run-mode",
                                   "actor-control", "set-run-mode", "movement"},
              "run click cycles signed mode and refreshes movement");
    }
    Runtime wrapped;
    wrapped.run_mode = std::numeric_limits<std::int32_t>::max();
    pal4::MinimapSceneUiActions::AdvanceRunMode(
        0x5000u, 0xc000u, wrapped);
    Check(wrapped.next_run_mode == std::numeric_limits<std::int32_t>::min(),
          "run mode increment preserves 32-bit wraparound");

    pal4::MinimapSceneUiState input_state;
    Runtime input;
    input.input_state = &input_state;
    Check(pal4::MinimapSceneUiActions::SetFlagTrue(
              input_state, 0xffffffffu) == 1u &&
              input_state.ReadByte(0xf6u) == 1u,
          "mouse-down helper sets scene flag");
    Check(pal4::MinimapSceneUiActions::SetFlagFalseAndResetInput(
              input_state, 0u, input) == 1u &&
              input_state.ReadByte(0xf6u) == 0u &&
              input.calls == std::vector<std::string>{
                                 "input-manager", "input-reset"},
          "mouse-up helper clears flag before resetting input manager");
}
