#include "pal4/magic_window_panel_events.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

pal4::MagicWindowPanelState Panel() {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t offset = 0; offset < 0x170u; offset += 4u)
        panel.At(offset) = 0x1000u + offset;
    return panel;
}

struct Runtime final : pal4::MagicWindowPanelEventsRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> selected_queries;
    std::int32_t selected_row{-1};
    std::int32_t selected_stunt_row{-1};
    bool primary_visible{true};
    bool secondary_visible{true};

    bool IsSelected(std::uint32_t button) override {
        selected_queries.push_back(button);
        return (selected_row >= 0 &&
                button == 0x10c0u + 4u * static_cast<std::uint32_t>(selected_row)) ||
               (selected_stunt_row >= 0 &&
                button == 0x1108u +
                              4u * static_cast<std::uint32_t>(selected_stunt_row));
    }
    void HandleMouseEvent(std::uint32_t window) override {
        calls.push_back("mouse:" + std::to_string(window));
    }
    void RemoveActive(std::uint32_t member) override {
        calls.push_back("remove:" + std::to_string(member));
    }
    void SetStuntPanelVisible(std::uint32_t owner, bool visible) override {
        Check(owner == 0x3000u && !visible, "close hides owner panel");
        calls.push_back("hide-panel");
    }
    void UpdateMagicDetails(std::uint32_t owner,
                            std::int32_t row) override {
        Check(owner == 0x3000u, "detail update owner");
        calls.push_back("details:" + std::to_string(row));
    }
    void UpdateStuntDetails(std::uint32_t owner,
                            std::int32_t row) override {
        Check(owner == 0x3000u, "stunt detail update owner");
        calls.push_back("stunt-details:" + std::to_string(row));
    }
    bool IsVisible(std::uint32_t window) override {
        calls.push_back("visible:" + std::to_string(window));
        if (window == 0x1080u) return primary_visible;
        Check(window == 0x1084u, "visibility query targets highlight controls");
        return secondary_visible;
    }
    void SetHoverFlag(std::uint32_t window, bool value) override {
        calls.push_back("flag:" + std::to_string(window) + ":" +
                        std::to_string(value));
    }
    void RequestRedraw(std::uint32_t window) override {
        calls.push_back("redraw:" + std::to_string(window));
    }
    pal4::MagicWindowPoint AbsolutePosition(std::uint32_t window) override {
        calls.push_back("position:" + std::to_string(window));
        return {10.0f, 20.0f};
    }
    pal4::MagicWindowPoint WindowSize(std::uint32_t window) override {
        calls.push_back("size:" + std::to_string(window));
        return {40.0f, 12.0f};
    }
    void DispatchStuntClick(
        std::uint32_t owner,
        const pal4::MagicWindowSyntheticClick& event) override {
        Check(owner == 0x3000u && event.cursor.x == 30.0f &&
                  event.cursor.y == 26.0f && event.guard == 0u &&
                  event.click_count == 1u,
              "synthetic click payload and button center");
        calls.push_back("dispatch:" + std::to_string(event.target));
    }
};

pal4::MagicWindowPanelEventState State(bool active = false) {
    return {.panel_owner = 0x3000u, .highlight_active = active};
}

bool Handle(std::int32_t code, pal4::MagicWindowPanelEventState& state,
            Runtime& runtime) {
    return pal4::MagicWindowPanelEvents::HandleEvent(
        0x4000u, code, state, Panel(), runtime);
}

bool HandleStunt(std::int32_t code,
                 pal4::MagicWindowPanelEventState& state,
                 Runtime& runtime) {
    return pal4::MagicWindowPanelEvents::HandleStuntRowsEvent(
        0x4000u, code, state, Panel(), runtime);
}

void TestDirectPanelClick() {
    Runtime runtime;
    for (const auto code : {-1, 0, 1, 8, 200})
        Check(!pal4::MagicWindowPanelEvents::HandlePanelClick(
                  code, Panel(), runtime), "out-of-range click returns false");
    Check(runtime.calls.empty(), "out-of-range click has no side effect");
    for (std::int32_t code = 2; code < 8; ++code)
        Check(pal4::MagicWindowPanelEvents::HandlePanelClick(
                  code, Panel(), runtime), "in-range click returns true");
    for (std::uint32_t index = 0; index < 6u; ++index)
        Check(runtime.calls[index] ==
                  "mouse:" + std::to_string(0x10a8u + 4u * index),
              "navigation-subobject shortcut maps event2-7 to full-page categories+a8..bc");
}

void TestHighlightAndReset() {
    Runtime runtime;
    auto state = State();
    pal4::MagicWindowPanelEvents::SetHighlight(true, state, Panel(), runtime);
    Check(state.highlight_active && runtime.calls == std::vector<std::string>{
                                        "visible:4224", "visible:4228",
                                        "flag:4228:1", "redraw:4228"},
          "visible highlight sets secondary hover flag");

    Runtime hidden_primary;
    hidden_primary.primary_visible = false;
    state = State();
    pal4::MagicWindowPanelEvents::SetHighlight(
        true, state, Panel(), hidden_primary);
    Check(!state.highlight_active &&
              hidden_primary.calls.back() == "redraw:4228",
          "hidden primary clears stored mode but still updates visible secondary");

    Runtime hidden_secondary;
    hidden_secondary.secondary_visible = false;
    state = State();
    pal4::MagicWindowPanelEvents::SetHighlight(
        true, state, Panel(), hidden_secondary);
    Check(state.highlight_active &&
              hidden_secondary.calls == std::vector<std::string>{
                                            "visible:4224", "visible:4228"},
          "hidden secondary skips hover flag and redraw");

    Runtime reset;
    state = State(true);
    Check(pal4::MagicWindowPanelEvents::ResetHighlight(state, Panel(), reset) &&
              !state.highlight_active, "reset clears stored highlight");
    Check(reset.calls == std::vector<std::string>{
                             "visible:4224", "flag:4228:0", "redraw:4228",
                             "flag:4224:0", "redraw:4224"},
          "reset clears secondary then primary regardless of visibility");
}

void TestCloseAndNavigation() {
    Runtime close;
    auto state = State();
    Check(Handle(1, state, close) &&
              close.calls == std::vector<std::string>{
                                 "remove:16384", "hide-panel"},
          "close removes member before hiding panel");

    Runtime up;
    up.selected_row = 3;
    state = State();
    Check(Handle(200, state, up) &&
              up.calls == std::vector<std::string>{"details:2"} &&
              up.selected_queries.size() == 4u,
          "up moves selected row one position back");
    Runtime up_first;
    up_first.selected_row = 0;
    state = State();
    Check(Handle(200, state, up_first) && state.highlight_active &&
              up_first.calls.back() == "redraw:4228",
          "up from first row enters highlight mode");
    Runtime up_active;
    state = State(true);
    Check(Handle(200, state, up_active) && up_active.calls.empty() &&
              up_active.selected_queries.empty(),
          "up is inert while highlighted");
    Runtime up_none;
    state = State();
    Check(Handle(200, state, up_none) && up_none.calls.empty() &&
              up_none.selected_queries.size() == 6u,
          "up with no row selected does nothing");

    Runtime down;
    down.selected_row = 4;
    state = State();
    Check(Handle(208, state, down) &&
              down.calls == std::vector<std::string>{"details:5"},
          "down moves to final row");
    Runtime down_last;
    down_last.selected_row = 5;
    state = State();
    Check(Handle(208, state, down_last) && down_last.calls.empty(),
          "down stops at final row");
    Runtime down_active;
    state = State(true);
    Check(Handle(208, state, down_active) && !state.highlight_active &&
              down_active.calls.front() == "details:0" &&
              down_active.calls.back() == "redraw:4224" &&
              down_active.selected_queries.empty(),
          "down from highlight selects first row then clears highlight");
}

void TestSideAndConfirmEvents() {
    Runtime left;
    auto state = State(true);
    Check(Handle(203, state, left) &&
              left.calls == std::vector<std::string>{
                                "flag:4228:0", "redraw:4228", "mouse:4224"},
          "left event clears secondary hover then clicks primary");
    Runtime right;
    state = State(true);
    Check(Handle(205, state, right) &&
              right.calls == std::vector<std::string>{
                                 "flag:4224:0", "redraw:4224", "mouse:4228"},
          "right event clears primary hover then clicks secondary");
    Runtime inactive;
    state = State();
    Check(Handle(203, state, inactive) && Handle(205, state, inactive) &&
              inactive.calls.empty(), "side events are inert without highlight");

    Runtime confirm;
    confirm.selected_row = 3;
    state = State();
    Check(Handle(28, state, confirm) &&
              confirm.calls == std::vector<std::string>{"mouse:4348"},
          "confirm clicks selected action button at +0xf0");
    Runtime no_selection;
    Check(Handle(28, state, no_selection) && no_selection.calls.empty(),
          "confirm without selected row is handled without click");
    Runtime unknown;
    Check(!Handle(27, state, unknown) && unknown.calls.empty(),
          "unknown code returns false");
}

void TestStuntRowNavigation() {
    Runtime up;
    up.selected_stunt_row = 4;
    auto state = State();
    Check(HandleStunt(200, state, up) &&
              up.calls == std::vector<std::string>{"stunt-details:3"} &&
              up.selected_queries.size() == 5u,
          "stunt up selects preceding row");
    Runtime up_first;
    up_first.selected_stunt_row = 0;
    state = State();
    Check(HandleStunt(200, state, up_first) && state.highlight_active &&
              up_first.calls.back() == "redraw:4228",
          "stunt up from first row enters highlight");
    Runtime up_active;
    state = State(true);
    Check(HandleStunt(200, state, up_active) &&
              up_active.selected_queries.empty(),
          "stunt up ignores selected rows while highlighted");

    Runtime down;
    down.selected_stunt_row = 5;
    state = State();
    Check(HandleStunt(208, state, down) &&
              down.calls == std::vector<std::string>{"stunt-details:6"},
          "stunt down reaches seventh row");
    Runtime down_last;
    down_last.selected_stunt_row = 6;
    state = State();
    Check(HandleStunt(208, state, down_last) && down_last.calls.empty(),
          "stunt down stops at seventh row");
    Runtime down_active;
    state = State(true);
    Check(HandleStunt(208, state, down_active) && !state.highlight_active &&
              down_active.calls.front() == "stunt-details:0" &&
              down_active.calls.back() == "redraw:4224",
          "stunt down exits highlight through first row");
}

void TestStuntRowConfirmAndFallback() {
    for (std::uint32_t row = 0; row < 7u; ++row) {
        Runtime runtime;
        runtime.selected_stunt_row = static_cast<std::int32_t>(row);
        auto state = State();
        Check(HandleStunt(28, state, runtime) &&
                  runtime.calls == std::vector<std::string>{
                                       "position:" + std::to_string(0x1108u + 4u * row),
                                       "size:" + std::to_string(0x1108u + 4u * row),
                                       "dispatch:" + std::to_string(0x1108u + 4u * row)},
              "each stunt row dispatches a centered synthetic click");
    }
    Runtime no_selection;
    auto state = State();
    Check(HandleStunt(28, state, no_selection) && no_selection.calls.empty(),
          "confirm with no stunt selection performs no dispatch");
    Runtime close;
    Check(HandleStunt(1, state, close) &&
              close.calls == std::vector<std::string>{
                                 "remove:16384", "hide-panel"},
          "stunt list falls back to common close event");
    Runtime unknown;
    Check(!HandleStunt(99, state, unknown) && unknown.calls.empty(),
          "unknown stunt event follows common unhandled path");
}

}  // namespace

int main() {
    TestDirectPanelClick();
    TestHighlightAndReset();
    TestCloseAndNavigation();
    TestSideAndConfirmEvents();
    TestStuntRowNavigation();
    TestStuntRowConfirmAndFallback();
}
