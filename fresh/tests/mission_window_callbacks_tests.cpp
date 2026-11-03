#include "pal4/mission_window_callbacks.h"
#include "pal4/mission_window_input.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMissionWindowCallbacksRuntime {
    std::vector<std::string> calls;

    void InsertActiveMember(pal4::MissionWindowState&,
                            std::uint32_t offset) override {
        calls.emplace_back("insert:" + std::to_string(offset));
    }
    void PopulateMissionList(pal4::MissionWindowState&) override {
        calls.emplace_back("mission");
    }
    void PopulateQuestList(pal4::MissionWindowState&) override {
        calls.emplace_back("quest");
    }
    void UpdateDetailsPanel(pal4::MissionWindowState&) override {
        calls.emplace_back("details");
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x8400u, "mask visibility target");
        calls.emplace_back(visible ? "show-mask" : "hide-mask");
    }
    void SetAlwaysOnTop(std::uint32_t window, bool value) override {
        Check(window == 0x5400u, "mission list top target");
        calls.emplace_back(value ? "top" : "not-top");
    }
    void HandleMouseEvent(std::uint32_t window) override {
        Check(window == 0x8400u, "mask mouse event target");
        calls.emplace_back("mouse");
    }
};

pal4::MissionWindowState MakeState() {
    pal4::MissionWindowState state{};
    state.Write(0x54u, 0x5400u);
    state.Write(0x84u, 0x8400u);
    return state;
}

}  // namespace

int main() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::MissionWindowInput::TabShortcut(state, 2u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"insert:64", "mission"},
          "task shortcut uses existing population and activity callback");
    runtime.calls.clear();
    Check(pal4::MissionWindowInput::TabShortcut(state, 3u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "insert:64", "quest", "hide-mask", "top"},
          "chapter shortcut preserves mask and stacking reset");
    runtime.calls.clear();
    for (const auto code : {1u, 28u, 200u, 208u})
        Check(pal4::MissionWindowInput::TabShortcut(state, code, runtime) == 0u &&
                  runtime.calls.empty(), "shortcut leaves unrelated keys unhandled");
    Check(pal4::MissionWindowCallbacks::OpenMission(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "insert:64", "mission"},
          "mission direct callback order");
    runtime.calls.clear();
    Check(pal4::MissionWindowCallbacks::MissionClicked(
              state, 0xdeadbeefu, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "insert:64", "mission"},
          "mission click forwards without using event");
    runtime.calls.clear();
    Check(pal4::MissionWindowCallbacks::OpenQuest(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "insert:64", "quest", "hide-mask", "top"},
          "quest direct callback order");
    runtime.calls.clear();
    Check(pal4::MissionWindowCallbacks::QuestClicked(
              state, 0xdeadbeefu, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "insert:64", "quest", "hide-mask", "top"},
          "quest click forwards without using event");
    runtime.calls.clear();
    Check(pal4::MissionWindowCallbacks::HoverChanged(
              state, 0xdeadbeefu, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"details"},
          "hover callback updates details");

    state.bytes[0x88u] = 1u;
    runtime.calls.clear();
    Check(pal4::MissionWindowCallbacks::ListMouseClick(
              state, 0xdeadbeefu, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "details", "insert:64"},
          "mission click inserts mission member after details");
    state.bytes[0x88u] = 0u;
    runtime.calls.clear();
    Check(pal4::MissionWindowCallbacks::ListMouseClick(
              state, 0xdeadbeefu, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "details", "insert:72", "show-mask", "not-top",
                  "mouse"},
          "chapter click shows mask and dispatches mouse event");
    std::cout << "Mission window callback tests passed\n";
}
