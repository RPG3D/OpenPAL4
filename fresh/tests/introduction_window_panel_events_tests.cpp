#include "pal4/introduction_window_panel_events.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IntroductionWindowPanelEventsRuntimeStub {
    std::uint32_t code{};
    std::uint32_t selected_window{};
    std::uint32_t clicked_window{};
    std::uint32_t removed_collection{};
    std::uint32_t removed_member{};
    std::uint32_t scroll_window{};
    std::uint32_t scroll_object{};
    float position = 35.0f;
    float new_position{};
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::pair<std::uint32_t, bool>> enabled;
    std::vector<std::uint32_t> examined;
    std::vector<std::string> calls;

    std::uint32_t ReadEventCode(std::uint32_t member) override {
        Check(member == 0x7010u, "event code is read at +0x10");
        calls.emplace_back("event");
        return code;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    void RemoveActive(std::uint32_t collection,
                      std::uint32_t member) override {
        removed_collection = collection;
        removed_member = member;
        calls.emplace_back("remove");
    }
    void SetVisible(std::uint32_t window, bool value) override {
        visibility.emplace_back(window, value);
        calls.emplace_back("visible");
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        enabled.emplace_back(window, value);
        calls.emplace_back("enabled");
    }
    bool IsVisible(std::uint32_t window) override {
        examined.push_back(window);
        calls.emplace_back("is-visible");
        return window == selected_window;
    }
    bool IsDisabled(std::uint32_t window) override {
        calls.emplace_back("is-disabled");
        return window == selected_window;
    }
    void HandleMouseEvent(std::uint32_t window) override {
        clicked_window = window;
        calls.emplace_back("mouse");
    }
    std::uint32_t ScrollbarOf(std::uint32_t window) override {
        scroll_window = window;
        calls.emplace_back("scrollbar");
        return 0x5000u;
    }
    float ScrollPosition(std::uint32_t scrollbar) override {
        scroll_object = scrollbar;
        calls.emplace_back("position");
        return position;
    }
    void SetScrollPosition(std::uint32_t scrollbar,
                           float value) override {
        Check(scrollbar == 0x5000u, "same scrollbar is updated");
        new_position = value;
        calls.emplace_back("set-position");
    }
};

pal4::IntroductionWindowState MakeState() {
    pal4::IntroductionWindowState state{};
    state.Write(0x80u, 0x8000u);
    state.Write(0x88u, 0x8800u);
    state.Write(0x8cu, 0x8c00u);
    state.Write(0x90u, 0x9000u);
    state.Write(0x94u, 0x9400u);
    state.Write(0x98u, 0x9800u);
    state.Write(0x9cu, 0x9c00u);
    state.Write(0xa0u, 0xa000u);
    state.Write(0xa4u, 0xa400u);
    state.Write(0xa8u, 0xa800u);
    state.Write(0xacu, 0xac00u);
    return state;
}

void CheckClose() {
    auto state = MakeState();
    Runtime runtime;
    runtime.code = 1u;
    Check(pal4::IntroductionWindowPanelEvents::Handle(
              state, 0x10b0u, 0x7000u, runtime) &&
              runtime.removed_collection == 0x9134u &&
              runtime.removed_member == 0x10b0u &&
              runtime.visibility ==
                  std::vector<std::pair<std::uint32_t, bool>>{
                      {0xac00u, false}} &&
              runtime.enabled ==
                  std::vector<std::pair<std::uint32_t, bool>>{
                      {0x8800u, true}, {0x8c00u, true},
                      {0x9000u, true}, {0x9400u, true}} &&
              runtime.calls.front() == "event" &&
              runtime.calls[1] == "manager" &&
              runtime.calls[2] == "remove",
          "close removes active member then hides panel");
}

void CheckNavigation() {
    auto state = MakeState();
    Runtime runtime;
    runtime.code = 203u;
    runtime.selected_window = 0xa000u;
    Check(pal4::IntroductionWindowPanelEvents::Handle(
              state, 0x10b0u, 0x7000u, runtime) &&
              runtime.examined ==
                  std::vector<std::uint32_t>{
                      0xa800u, 0xa400u, 0xa000u} &&
              runtime.clicked_window == 0x9c00u,
          "previous navigation scans backward to selected subwindow");

    runtime = Runtime{};
    runtime.code = 205u;
    runtime.selected_window = 0xa000u;
    Check(pal4::IntroductionWindowPanelEvents::Handle(
              state, 0x10b0u, 0x7000u, runtime) &&
              runtime.examined ==
                  std::vector<std::uint32_t>{
                      0x9800u, 0x9c00u, 0xa000u} &&
              runtime.clicked_window == 0xa400u,
          "next navigation scans forward to selected subwindow");

    runtime = Runtime{};
    runtime.code = 203u;
    runtime.selected_window = 0x9800u;
    Check(pal4::IntroductionWindowPanelEvents::Handle(
              state, 0x10b0u, 0x7000u, runtime) &&
              runtime.clicked_window == 0u,
          "first subwindow has no previous neighbor");

    runtime = Runtime{};
    runtime.code = 205u;
    runtime.selected_window = 0xa800u;
    Check(pal4::IntroductionWindowPanelEvents::Handle(
              state, 0x10b0u, 0x7000u, runtime) &&
              runtime.clicked_window == 0u,
          "last subwindow has no next neighbor");

    runtime = Runtime{};
    runtime.code = 203u;
    Check(pal4::IntroductionWindowPanelEvents::Handle(
              state, 0x10b0u, 0x7000u, runtime) &&
              runtime.examined.size() == 5u &&
              runtime.clicked_window == 0u,
          "no selected subwindow consumes navigation without a click");
}

void CheckScrollingAndDefault() {
    auto state = MakeState();
    Runtime runtime;
    runtime.code = 200u;
    Check(pal4::IntroductionWindowPanelEvents::Handle(
              state, 0x10b0u, 0x7000u, runtime) &&
              runtime.scroll_window == 0x8000u &&
              runtime.scroll_object == 0x5000u &&
              runtime.new_position == 25.0f,
          "scroll-up subtracts ten from current position");

    runtime = Runtime{};
    runtime.code = 208u;
    Check(pal4::IntroductionWindowPanelEvents::Handle(
              state, 0x10b0u, 0x7000u, runtime) &&
              runtime.new_position == 45.0f,
          "scroll-down adds ten to current position");

    runtime = Runtime{};
    runtime.code = 999u;
    Check(!pal4::IntroductionWindowPanelEvents::Handle(
              state, 0x10b0u, 0x7000u, runtime) &&
              runtime.calls == std::vector<std::string>{"event"},
          "unknown event returns false without side effects");
}

}  // namespace

int main() {
    CheckClose();
    CheckNavigation();
    CheckScrollingAndDefault();
    std::cout << "Introduction window panel events tests passed\n";
}
