#include "pal4/property_window_panel_events.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowPanelEventsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t rows = 0u;
    std::uint32_t hover = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void UpdateCategoryPanel(pal4::PropertyWindowState& state) override {
        calls.push_back("category:" + std::to_string(state.Read(0x6cu)));
    }
    void RemoveActiveMember(std::uint32_t callback) override {
        Check(callback == 0x9000u, "close removes callback object");
        calls.emplace_back("remove-active");
    }
    void HideItemPanel(std::uint32_t owner) override {
        Check(owner == 0x8000u, "close hides callback owner");
        calls.emplace_back("hide");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x7000u, "row count reads owner's list +0x58");
        calls.emplace_back("rows");
        return rows;
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x7000u, "navigation queries same list");
        calls.emplace_back("hover");
        return hover;
    }
    void MoveHoverUp(std::uint32_t list) override {
        Check(list == 0x7000u, "up moves hover on item list");
        calls.emplace_back("up");
    }
    void MoveHoverDown(std::uint32_t list) override {
        Check(list == 0x7000u, "down moves hover on item list");
        calls.emplace_back("down");
    }
    void ShowItemSelection(
        std::uint32_t owner,
        const std::array<float, 2>& position) override {
        Check(owner == 0x8000u &&
                  std::bit_cast<std::uint32_t>(position[0]) == 0x43c80000u &&
                  std::bit_cast<std::uint32_t>(position[1]) == 0x42c80000u,
              "selection uses owner and source 400x100 coordinates");
        calls.emplace_back("select");
    }
};

void TestCategoryClickAndConstruction() {
    pal4::PropertyWindowState state;
    state.Write(0x6cu, 9u);
    Runtime runtime;
    runtime.words[0x7010u] = 2u;
    Check(pal4::PropertyWindowPanelEvents::HandleCategoryClick(
              state, 0x7000u, runtime) && state.Read(0x6cu) == 5u,
          "code two selects recent category");
    Check(runtime.calls == std::vector<std::string>{
              "read:28688", "category:5"},
          "valid click updates category panel after state write");

    for (std::uint32_t code = 3u; code < 8u; ++code) {
        runtime.words[0x7010u] = code;
        Check(pal4::PropertyWindowPanelEvents::HandleCategoryClick(
                  state, 0x7000u, runtime) &&
                  state.Read(0x6cu) == code - 3u,
              "codes three through seven map to categories zero through four");
    }
    for (const auto code : {0u, 1u, 8u, 0xffffffffu}) {
        runtime.words[0x7010u] = code;
        Check(!pal4::PropertyWindowPanelEvents::HandleCategoryClick(
                  state, 0x7000u, runtime) &&
                  state.Read(0x6cu) == 4u,
              "invalid category code returns false without mutation");
    }

    runtime.calls.clear();
    Check(pal4::PropertyWindowPanelEvents::ConstructCallback(
              0x9000u, runtime) == 0x9000u &&
          runtime.words[0x9000u] ==
              pal4::PropertyWindowPanelEvents::kCallbackVtable,
          "callback constructor writes vtable and returns object");
    Check(runtime.calls == std::vector<std::string>{"write:36864"},
          "callback constructor has one write");
}

void TestPanelEvents() {
    Runtime runtime;
    runtime.words[0x9004u] = 0x8000u;
    runtime.words[0x8058u] = 0x7000u;
    runtime.words[0xa010u] = 1u;
    Check(pal4::PropertyWindowPanelEvents::HandleItemPanelEvent(
              0x9000u, 0xa000u, runtime), "close event returns true");
    Check(runtime.calls == std::vector<std::string>{
              "read:40976", "remove-active", "read:36868", "hide"},
          "close removes active member before hiding panel");

    runtime.calls.clear();
    runtime.words[0xa010u] = 200u;
    Check(!pal4::PropertyWindowPanelEvents::HandleItemPanelEvent(
              0x9000u, 0xa000u, runtime),
          "empty list returns false before navigation");
    Check(runtime.calls == std::vector<std::string>{
              "read:40976", "read:36868", "read:32856", "rows"},
          "empty list skips hover lookup");

    runtime.calls.clear();
    runtime.rows = 2u;
    Check(pal4::PropertyWindowPanelEvents::HandleItemPanelEvent(
              0x9000u, 0xa000u, runtime),
          "up event with no hover still returns true");
    Check(runtime.calls.back() == "hover",
          "up event without hover does not move selection");

    runtime.calls.clear();
    runtime.hover = 0x8100u;
    Check(pal4::PropertyWindowPanelEvents::HandleItemPanelEvent(
              0x9000u, 0xa000u, runtime) &&
          runtime.calls.back() == "up",
          "up event moves existing hover");

    runtime.calls.clear();
    runtime.words[0xa010u] = 208u;
    Check(pal4::PropertyWindowPanelEvents::HandleItemPanelEvent(
              0x9000u, 0xa000u, runtime) &&
          runtime.calls.back() == "down",
          "down event moves existing hover");

    runtime.calls.clear();
    runtime.words[0xa010u] = 28u;
    Check(pal4::PropertyWindowPanelEvents::HandleItemPanelEvent(
              0x9000u, 0xa000u, runtime) &&
          runtime.calls.back() == "select",
          "selection event uses synthetic coordinates");

    runtime.calls.clear();
    runtime.words[0xa010u] = 29u;
    Check(!pal4::PropertyWindowPanelEvents::HandleItemPanelEvent(
              0x9000u, 0xa000u, runtime) &&
          runtime.calls.back() == "rows",
          "unhandled code returns false after row-count gate");
}

}  // namespace

int main() {
    TestCategoryClickAndConstruction();
    TestPanelEvents();
}
