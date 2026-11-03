#include "pal4/save_window_navigation.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::SaveWindowNavigationRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t rows{10u};
    std::uint32_t hover{0x9000u};
    std::int32_t expected_page{-1};
    bool fail_close{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t ConstructBaseEvent() override {
        calls.emplace_back("construct-event");
        return 0xa000u;
    }
    void DestroyBaseEvent(std::uint32_t event) override {
        Check(event == 0xa000u, "temporary close event destroyed");
        calls.emplace_back("destroy-event");
    }
    void Close(pal4::SaveWindowState&, std::uint32_t object,
               std::uint32_t event) override {
        Check(object == 0x6000u && event == 0xa000u,
              "close adjusts member to containing save window");
        calls.emplace_back("close");
        if (fail_close) throw std::runtime_error("close");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x5000u, "row count uses List");
        calls.emplace_back("rows");
        return rows;
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x5000u, "hover item uses List");
        calls.emplace_back("hover-item");
        return hover;
    }
    void UpHoverItem(std::uint32_t list) override {
        Check(list == 0x5000u, "up hover uses List");
        calls.emplace_back("up");
    }
    void DownHoverItem(std::uint32_t list) override {
        Check(list == 0x5000u, "down hover uses List");
        calls.emplace_back("down");
    }
    void PopulateSaveList(pal4::SaveWindowState&, std::int32_t page,
                          std::uint8_t reset) override {
        Check(page == expected_page && reset == 0u,
              "page navigation uses adjacent page without reset");
        calls.emplace_back("populate");
    }
    void DisplayHoverItemInfo(pal4::SaveWindowState&) override {
        calls.emplace_back("dialog");
    }
};

pal4::SaveWindowState State() {
    pal4::SaveWindowState state{};
    state.Write(0x54u, 0x5000u);
    state.Write(0x5cu, 0x6000u);
    return state;
}

Runtime Event(std::uint32_t code) {
    Runtime runtime;
    runtime.words[0x8010u] = code;
    runtime.words[0x54c0u] = 3u;
    runtime.words[0x6118u] = 3u;
    return runtime;
}

void CheckEvent(std::uint32_t code, std::uint32_t hover_index,
                std::int32_t page, std::string expected,
                std::uint8_t expected_result = 1u) {
    auto state = State();
    auto runtime = Event(code);
    runtime.words[0x54c0u] = hover_index;
    runtime.words[0x6118u] = static_cast<std::uint32_t>(page);
    runtime.expected_page = code == 203u ? page - 1 : page + 1;
    Check(pal4::SaveWindowNavigation::OnMemberEvent(
              state, 0x6028u, 0x8000u, runtime) == expected_result,
          "navigation event return");
    Check(runtime.calls.front() == "read" &&
              runtime.calls[1] == "rows" &&
              (expected.empty() || runtime.calls.back() == expected),
          "navigation dispatches expected action after row check");
    if (expected.empty())
        Check(runtime.calls.back() != "up" &&
                  runtime.calls.back() != "down" &&
                  runtime.calls.back() != "populate" &&
                  runtime.calls.back() != "dialog",
              "boundary event makes no state-changing call");
}

void CheckNavigation() {
    CheckEvent(200u, 3u, 3, "up");
    CheckEvent(200u, 0u, 3, "");
    CheckEvent(208u, 3u, 3, "down");
    CheckEvent(208u, 9u, 3, "");
    CheckEvent(203u, 3u, 3, "populate");
    CheckEvent(203u, 3u, 0, "");
    CheckEvent(205u, 3u, 3, "populate");
    CheckEvent(205u, 3u, 9, "");
    CheckEvent(28u, 3u, 3, "dialog");
    CheckEvent(999u, 3u, 3, "", 0u);
    auto state = State();
    auto runtime = Event(200u);
    runtime.hover = 0u;
    Check(pal4::SaveWindowNavigation::OnMemberEvent(
              state, 0x6028u, 0x8000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read", "rows", "hover-item"},
          "missing hover item consumes up event without moving");
    runtime = Event(28u);
    runtime.rows = 0u;
    Check(pal4::SaveWindowNavigation::OnMemberEvent(
              state, 0x6028u, 0x8000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"read", "rows"},
          "empty list rejects non-close events");
}

void CheckClose() {
    auto state = State();
    auto runtime = Event(1u);
    runtime.rows = 0u;
    Check(pal4::SaveWindowNavigation::OnMemberEvent(
              state, 0x6028u, 0x8000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read", "construct-event", "close", "destroy-event"},
          "close precedes row-count gate");
    runtime.calls.clear();
    runtime.fail_close = true;
    try {
        (void)pal4::SaveWindowNavigation::OnMemberEvent(
            state, 0x6028u, 0x8000u, runtime);
        Check(false, "close failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "destroy-event",
              "temporary close event released on failure");
    }
}

}  // namespace

int main() {
    CheckNavigation();
    CheckClose();
    std::cout << "Save window navigation tests passed\n";
}
