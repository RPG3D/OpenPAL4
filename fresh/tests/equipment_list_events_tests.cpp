#include "pal4/equipment_list_events.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::EquipmentListEventsRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t rows{3u};
    std::uint32_t hovered{0x9000u};
    std::uint32_t row_index{1u};
    std::uint32_t enabled_control{0u};
    bool left_visible{true};
    bool right_visible{true};

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5010u, "event code read at +0x10");
        calls.emplace_back("event");
        return words[address];
    }
    void RemovePrimaryMember() override {
        calls.emplace_back("remove-primary");
    }
    void InsertSecondaryMember() override {
        calls.emplace_back("insert-secondary");
    }
    void SetEquipmentVisible(pal4::EquipmentWindowState&, bool value) override {
        Check(!value, "close hides equipment controls");
        calls.emplace_back("hide-equipment");
    }
    bool IsDisabled(std::uint32_t window) override {
        calls.emplace_back("disabled:" + std::to_string(window));
        return window != enabled_control;
    }
    bool IsVisible(std::uint32_t window) override {
        Check(window == 0x4800u || window == 0x4c00u,
              "highlight checks navigation button visibility");
        calls.emplace_back("visible:" + std::to_string(window));
        return window == 0x4800u ? left_visible : right_visible;
    }
    void SetHoverFlag(std::uint32_t window, bool hovered) override {
        calls.emplace_back("hover-flag:" + std::to_string(window) +
                           (hovered ? ":1" : ":0"));
    }
    void RequestRedraw(std::uint32_t window) override {
        calls.emplace_back("redraw:" + std::to_string(window));
    }
    void HandleMouseEvent(std::uint32_t window) override {
        calls.emplace_back("mouse:" + std::to_string(window));
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x4000u, "row count list");
        calls.emplace_back("rows");
        return rows;
    }
    std::uint32_t HoveredItem(std::uint32_t list) override {
        Check(list == 0x4000u, "hover item list");
        calls.emplace_back("hover");
        return hovered;
    }
    std::uint32_t HoverRowIndex(std::uint32_t list) override {
        Check(list == 0x4000u, "hover index list");
        calls.emplace_back("index");
        return row_index;
    }
    void SetHoverRowIndex(std::uint32_t list,
                          std::uint32_t row) override {
        Check(list == 0x4000u && row == 0u,
              "focus transition selects first row");
        calls.emplace_back("select-first");
    }
    void UpHoverItem(std::uint32_t list) override {
        Check(list == 0x4000u, "move up list");
        calls.emplace_back("up");
    }
    void DownHoverItem(std::uint32_t list) override {
        Check(list == 0x4000u, "move down list");
        calls.emplace_back("down");
    }
    void HandleItemClick(pal4::EquipmentWindowState&,
                         float x, float y) override {
        Check(x == 400.0f && y == 100.0f,
              "enter key uses IDA synthetic pointer coordinates");
        calls.emplace_back("click");
    }
};

pal4::EquipmentWindowState MakeState() {
    pal4::EquipmentWindowState state{};
    state.Write(0x40u, 0x4000u);
    state.Write(0x48u, 0x4800u);
    state.Write(0x4cu, 0x4c00u);
    for (std::uint32_t index = 0u; index < 6u; ++index)
        state.Write(0x98u + 4u * index, 0x9800u + index);
    return state;
}

Runtime MakeRuntime(std::uint32_t code) {
    Runtime runtime;
    runtime.words[0x5010u] = code;
    return runtime;
}

bool Handle(pal4::EquipmentWindowState& state,
            pal4::EquipmentListMemberState& member,
            Runtime& runtime) {
    return pal4::EquipmentListEvents::Handle(
        state, member, 0x5000u, runtime);
}

void TestCloseAndFocusTransition() {
    auto state = MakeState();
    pal4::EquipmentListMemberState member{};
    auto runtime = MakeRuntime(1u);
    runtime.rows = 0u;
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "remove-primary", "hide-equipment"},
          "close removes member and hides controls without row check");

    runtime = MakeRuntime(15u);
    runtime.rows = 0u;
    Check(Handle(state, member, runtime) && runtime.calls.size() == 7u &&
              runtime.calls.back() == "disabled:38917",
          "all disabled controls consume focus event without insertion");

    runtime = MakeRuntime(15u);
    runtime.enabled_control = 0x9802u;
    Check(Handle(state, member, runtime), "focus event returns true");
    Check(runtime.calls == std::vector<std::string>{
              "event", "disabled:38912", "disabled:38913",
              "disabled:38914", "insert-secondary", "hover-flag:18432:0",
              "redraw:18432", "hover-flag:19456:0", "redraw:19456"},
          "first enabled equipment control moves focus and clears nav hover");
}

void TestEmptyAndUp() {
    auto state = MakeState();
    pal4::EquipmentListMemberState member{};
    for (const auto code : {200u, 208u, 203u, 205u, 28u, 99u}) {
        auto runtime = MakeRuntime(code);
        runtime.rows = 0u;
        Check(!Handle(state, member, runtime) &&
                  runtime.calls == std::vector<std::string>{"event", "rows"},
              "empty list declines non-focus events");
    }

    auto runtime = MakeRuntime(200u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "rows", "hover", "index", "up"},
          "up key moves hovered row up");
    runtime = MakeRuntime(200u);
    runtime.row_index = 0u;
    Check(Handle(state, member, runtime) && member.equipment_highlighted &&
              runtime.calls == std::vector<std::string>{
                  "event", "rows", "hover", "index", "visible:18432",
                  "visible:19456", "hover-flag:19456:1",
                  "redraw:19456"},
          "up from first row moves highlight to equipment controls");

    member.equipment_highlighted = false;
    runtime = MakeRuntime(200u);
    runtime.hovered = 0u;
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "rows", "hover"},
          "up with no hover does nothing");
    member.equipment_highlighted = true;
    runtime = MakeRuntime(200u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{"event", "rows"},
          "up while equipment highlighted does nothing");
}

void TestDown() {
    auto state = MakeState();
    pal4::EquipmentListMemberState member{true};
    auto runtime = MakeRuntime(208u);
    Check(Handle(state, member, runtime) &&
              !member.equipment_highlighted &&
              runtime.calls == std::vector<std::string>{
                  "event", "rows", "rows", "visible:18432",
                  "hover-flag:19456:0", "redraw:19456",
                  "hover-flag:18432:0", "redraw:18432", "select-first"},
          "down from equipment controls returns to first row");

    runtime = MakeRuntime(208u);
    runtime.row_index = 1u;
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "rows", "hover", "index", "rows", "down"},
          "down moves hovered row when a next row exists");
    runtime = MakeRuntime(208u);
    runtime.row_index = 2u;
    Check(Handle(state, member, runtime) &&
              runtime.calls.back() == "rows" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "down") == runtime.calls.end(),
          "down from last row stays put");
    runtime = MakeRuntime(208u);
    runtime.hovered = 0u;
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "rows", "hover"},
          "down with no hovered item stays put");
}

void TestSidesEnterAndUnknown() {
    auto state = MakeState();
    pal4::EquipmentListMemberState member{true};
    auto runtime = MakeRuntime(203u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "rows", "hover-flag:19456:0",
                  "redraw:19456", "mouse:18432"},
          "left key transfers hover from right to left button");
    runtime = MakeRuntime(205u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "rows", "hover-flag:18432:0",
                  "redraw:18432", "mouse:19456"},
          "right key transfers hover from left to right button");

    member.equipment_highlighted = false;
    runtime = MakeRuntime(203u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{"event", "rows"},
          "left key outside equipment highlight has no side effect");
    runtime = MakeRuntime(28u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "rows", "click"},
          "enter invokes item click when list is highlighted");
    member.equipment_highlighted = true;
    runtime = MakeRuntime(28u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{"event", "rows"},
          "enter on equipment controls does not click list item");
    runtime = MakeRuntime(99u);
    Check(!Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{"event", "rows"},
          "unknown event is unhandled");
}

void TestHighlightAndReset() {
    auto state = MakeState();
    pal4::EquipmentListMemberState member{};
    auto runtime = MakeRuntime(0u);
    pal4::EquipmentListEvents::SetHighlight(state, member, true, runtime);
    Check(member.equipment_highlighted &&
              runtime.calls == std::vector<std::string>{
                  "visible:18432", "visible:19456",
                  "hover-flag:19456:1", "redraw:19456"},
          "highlight selects visible right navigation button");

    runtime = MakeRuntime(0u);
    runtime.left_visible = false;
    pal4::EquipmentListEvents::SetHighlight(state, member, true, runtime);
    Check(!member.equipment_highlighted &&
              runtime.calls == std::vector<std::string>{
                  "visible:18432", "visible:19456",
                  "hover-flag:19456:1", "redraw:19456"},
          "hidden left button clears state but still redraws visible right");

    runtime = MakeRuntime(0u);
    runtime.right_visible = false;
    pal4::EquipmentListEvents::SetHighlight(state, member, true, runtime);
    Check(member.equipment_highlighted &&
              runtime.calls == std::vector<std::string>{
                  "visible:18432", "visible:19456"},
          "hidden right button receives no hover or redraw");

    runtime = MakeRuntime(0u);
    Check(pal4::EquipmentListEvents::ResetHighlight(state, member, runtime) &&
              !member.equipment_highlighted &&
              runtime.calls == std::vector<std::string>{
                  "visible:18432", "hover-flag:19456:0",
                  "redraw:19456", "hover-flag:18432:0",
                  "redraw:18432"},
          "reset clears both navigation buttons in IDA order");
}

}  // namespace

int main() {
    TestCloseAndFocusTransition();
    TestEmptyAndUp();
    TestDown();
    TestSidesEnterAndUnknown();
    TestHighlightAndReset();
}
