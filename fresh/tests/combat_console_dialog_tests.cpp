#include "pal4/combat_console.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kConsole = 0x5000u;
constexpr std::uint32_t kEvent = 0x6000u;
constexpr std::uint32_t kActor = 0x7000u;
constexpr std::uint32_t kSystem = 0x8000u;
constexpr std::uint32_t kPanel = 0x9000u;
constexpr std::uint32_t kNames = 0xa000u;

struct IdIterator final : pal4::ICombatConsoleDialogIdIterator {
    std::vector<std::string>& calls;
    std::string label;
    std::vector<std::int32_t> ids;
    std::size_t position = 0u;

    IdIterator(std::vector<std::string>& log, std::string name,
               std::vector<std::int32_t> values)
        : calls(log), label(std::move(name)), ids(std::move(values)) {}
    ~IdIterator() override { calls.push_back("destroy-" + label); }
    bool AtEnd() override {
        calls.push_back("end-" + label);
        return position == ids.size();
    }
    std::int32_t CurrentId() override {
        calls.push_back("id-" + label);
        return ids.at(position);
    }
    void Advance() override {
        calls.push_back("next-" + label);
        ++position;
    }
};

struct Runtime final : pal4::ICombatConsoleDialogRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kActor + 804u, 2u}, {kSystem + 508u, 5u},
        {kConsole + 36u, 0xb000u}, {kConsole + 260u, kPanel},
        {kConsole + 264u, 0x9100u}, {kConsole + 296u, kNames},
        {kConsole + 300u, kNames + 64u}};
    std::vector<std::string> calls;
    std::vector<std::int32_t> character_ids{0, 1, 4, -1};
    std::vector<std::int32_t> speaker_ids{2};
    std::vector<pal4::CombatConsolePosition> positions;
    std::u32string text;
    float centered_x = 0.0f;
    bool ending = true;
    float edit_height = 20.0f;
    float panel_height = 100.0f;
    float edit_width = 40.0f;
    float panel_width = 200.0f;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read");
        return words.at(address);
    }
    std::uint32_t ActorFromEvent(std::uint32_t event) override {
        Check(event == kEvent, "dialog event");
        calls.push_back("actor");
        return kActor;
    }
    std::uint32_t CombatSystem() override {
        calls.push_back("system");
        return kSystem;
    }
    bool IsCombatEnding(std::uint32_t system) override {
        Check(system == kSystem, "combat state owner");
        calls.push_back("ending");
        return ending;
    }
    pal4::CombatConsolePosition GameConfigPosition() override {
        calls.push_back("config-position");
        return {100.0f, 200.0f};
    }
    pal4::CombatConsolePosition FrameCoordinates(
        pal4::CombatConsolePosition position) override {
        Check(position.x == 100.0f && position.y == 200.0f,
              "frame input");
        calls.push_back("frame");
        return {110.0f, 220.0f};
    }
    pal4::CombatConsolePosition ScreenToWindow(
        std::uint32_t parent,
        pal4::CombatConsolePosition position) override {
        Check(parent == 0xb000u, "dialog root window");
        calls.push_back("screen-to-window");
        return {position.x + 1.0f, position.y + 2.0f};
    }
    pal4::CombatConsolePosition AbsoluteToRelative(
        std::uint32_t parent,
        pal4::CombatConsolePosition position) override {
        Check(parent == 0xb000u && position.x == 32.0f &&
                  position.y == 32.0f,
              "32-pixel relative offset");
        calls.push_back("relative");
        return {3.0f, 4.0f};
    }
    std::uint32_t CurrentCombatCamera() override {
        calls.push_back("camera");
        return 0xc000u;
    }
    pal4::CombatConsolePosition ActorScreenPosition(
        std::uint32_t actor, std::uint32_t camera) override {
        Check(actor == kActor && camera == 0xc000u,
              "actor screen position inputs");
        calls.push_back("actor-position");
        return {300.0f, 200.0f};
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == kPanel && visible, "dialog panel shown");
        calls.push_back("visible");
    }
    void SetPosition(std::uint32_t window,
                     pal4::CombatConsolePosition position) override {
        Check(window == kPanel, "dialog panel positioning");
        calls.push_back("position");
        positions.push_back(position);
    }
    std::string ReadDialogText(std::uint32_t record) override {
        Check(record == kActor + 1032u, "dialog text record offset");
        calls.push_back("dialog-text");
        return "line";
    }
    std::unique_ptr<pal4::ICombatConsoleDialogIdIterator>
    CharacterNameIds(std::uint32_t actor) override {
        Check(actor == kActor, "character list actor");
        calls.push_back("character-ids");
        return std::make_unique<IdIterator>(calls, "characters",
                                             character_ids);
    }
    std::unique_ptr<pal4::ICombatConsoleDialogIdIterator>
    SpeakerNameIds(std::uint32_t actor) override {
        Check(actor == kActor, "speaker list actor");
        calls.push_back("speaker-ids");
        return std::make_unique<IdIterator>(calls, "speakers", speaker_ids);
    }
    std::string ReadTextRecord(std::uint32_t record) override {
        calls.push_back("name-record");
        if (record == kNames + 16u) return "A";
        Check(record == kNames + 32u, "name index and stride");
        return "\xe5\xa4\x8f";
    }
    std::string ConvertToUtf8(std::string_view source) override {
        calls.push_back("utf8");
        return std::string(source);
    }
    void SetText(std::uint32_t window,
                 std::u32string_view value) override {
        Check(window == 0x9100u, "dialog edit control");
        calls.push_back("set-text");
        text.assign(value);
    }
    float GetTextHeight(std::uint32_t edit) override {
        Check(edit == 0x9100u, "text-height edit");
        calls.push_back("layout");
        return 30.0f;
    }
    float GetMaxLineWidth(std::uint32_t edit) override {
        Check(edit == 0x9100u, "line-width edit");
        calls.push_back("line-width");
        return 60.0f;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.push_back("dimension-read");
        if (address == 0x9100u + 272u) return edit_height;
        if (address == kPanel + 272u) return panel_height;
        if (address == 0x9100u + 268u) return edit_width;
        Check(address == kPanel + 268u, "panel width field");
        return panel_width;
    }
    void SetHeight(std::uint32_t window, std::uint32_t mode,
                   float value) override {
        Check(mode == 1u, "pixel height mode");
        if (window == 0x9100u) {
            calls.push_back("edit-height");
            edit_height = value;
        } else {
            Check(window == kPanel, "panel height target");
            calls.push_back("panel-height");
            panel_height = value;
        }
    }
    void SetWidth(std::uint32_t window, std::uint32_t mode,
                  float value) override {
        Check(mode == 1u, "pixel width mode");
        if (window == 0x9100u) {
            calls.push_back("edit-width");
            edit_width = value;
        } else {
            Check(window == kPanel, "panel width target");
            calls.push_back("panel-width");
            panel_width = value;
        }
    }
    std::int32_t DialogTotal(std::uint32_t actor,
                             std::uint32_t mode) override {
        Check(actor == kActor && mode == 0u, "total dialog call");
        calls.push_back("total");
        return 12;
    }
    std::int32_t DialogCurrentIndex(std::uint32_t actor,
                                    std::uint32_t mode,
                                    std::int32_t total) override {
        Check(actor == kActor && mode == 7u && total == 12,
              "current dialog call");
        calls.push_back("current");
        return 3;
    }
    float GetXPosition(std::uint32_t window) override {
        Check(window == kPanel, "dialog x query");
        calls.push_back("x");
        return 400.0f;
    }
    float GetWidth(std::uint32_t window) override {
        Check(window == kPanel, "dialog width query");
        calls.push_back("width");
        return panel_width;
    }
    void SetXPosition(std::uint32_t window, float x) override {
        Check(window == kPanel, "dialog center target");
        calls.push_back("center");
        centered_x = x;
    }
};

std::size_t First(const Runtime& runtime, const std::string& name) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 name);
    Check(found != runtime.calls.end(), "missing dialog operation");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void ScenarioBranch() {
    Runtime runtime;
    Check(pal4::CombatConsoleWindow::DisplayDialog(
              kConsole, kEvent, runtime), "scenario dialog shown");
    Check(runtime.positions.size() == 1u &&
              runtime.positions[0].x == 114.0f &&
              runtime.positions[0].y == 226.0f,
          "config position converts through frame, parent and 32-pixel offset");
    Check(runtime.text == U"line<br/>A\u590f",
          "dialog text appends both valid name records as Unicode");
    Check(runtime.edit_height == 35.0f && runtime.panel_height == 115.0f &&
              runtime.edit_width == 65.0f && runtime.panel_width == 225.0f,
          "scenario layout grows edit and panel from text dimensions");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "name-record") == 2 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "visible") == 2,
          "invalid IDs are skipped and panel is shown twice");
    Check(First(runtime, "frame") < First(runtime, "screen-to-window") &&
              First(runtime, "screen-to-window") < First(runtime, "relative") &&
              First(runtime, "character-ids") < First(runtime, "speaker-ids") &&
              First(runtime, "set-text") < First(runtime, "layout") &&
              First(runtime, "layout") < First(runtime, "destroy-speakers") &&
              First(runtime, "destroy-speakers") <
                  First(runtime, "destroy-characters"),
          "scenario work and iterator cleanup order");
}

void ScenarioStateGate() {
    Runtime runtime;
    runtime.words[kSystem + 508u] = 4u;
    Check(!pal4::CombatConsoleWindow::DisplayDialog(
              kConsole, kEvent, runtime) &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "visible") == 0,
          "scenario branch requires combat system state five");
}

void CombatBranchAndGates() {
    Runtime inactive;
    inactive.words[kActor + 804u] = 1u;
    inactive.ending = false;
    Check(!pal4::CombatConsoleWindow::DisplayDialog(
              kConsole, kEvent, inactive) &&
              std::count(inactive.calls.begin(), inactive.calls.end(),
                         "camera") == 0,
          "combat branch requires ending state");

    Runtime wrong_state;
    wrong_state.words[kActor + 804u] = 1u;
    wrong_state.words[kSystem + 508u] = 4u;
    Check(!pal4::CombatConsoleWindow::DisplayDialog(
              kConsole, kEvent, wrong_state) &&
              std::count(wrong_state.calls.begin(), wrong_state.calls.end(),
                         "camera") == 0,
          "combat branch also requires system state five");

    Runtime active;
    active.words[kActor + 804u] = 1u;
    Check(pal4::CombatConsoleWindow::DisplayDialog(
              kConsole, kEvent, active), "combat dialog shown");
    Check(active.positions.size() == 1u &&
              active.positions[0].x == 301.0f &&
              active.positions[0].y == 172.0f,
          "actor screen position moves upward by thirty before conversion");
    Check(active.text ==
              U"<colour red=255 green=248 blue=192><font name=dialog_simsun>"
              U"3/12</font></colour>" &&
              active.centered_x == 287.5f,
          "combat counter markup and horizontal centering");
    Check(First(active, "camera") < First(active, "actor-position") &&
              First(active, "visible") < First(active, "screen-to-window") &&
              First(active, "total") < First(active, "current") &&
              First(active, "layout") < First(active, "x") &&
              std::count(active.calls.begin(), active.calls.end(),
                         "visible") == 2,
          "combat layout and visibility order");
}

void LayoutInIsolation() {
    Runtime runtime;
    pal4::CombatConsoleWindow::UpdateDialogLayout(kConsole, runtime);
    Check(runtime.edit_height == 35.0f && runtime.panel_height == 115.0f &&
              runtime.edit_width == 65.0f && runtime.panel_width == 225.0f,
          "layout uses five-pixel padding and propagates size deltas");
    Check(First(runtime, "layout") < First(runtime, "edit-height") &&
              First(runtime, "edit-height") < First(runtime, "panel-height") &&
              First(runtime, "panel-height") < First(runtime, "line-width") &&
              First(runtime, "line-width") < First(runtime, "edit-width") &&
              First(runtime, "edit-width") < First(runtime, "panel-width"),
          "height phase completes before width phase");
}

}  // namespace

int main() {
    try {
        ScenarioBranch();
        ScenarioStateGate();
        CombatBranchAndGates();
        LayoutInIsolation();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
