#include "pal4/mission_window_story_selection.h"

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

struct Runtime final : pal4::IMissionWindowStorySelectionRuntime {
    std::vector<std::string> calls;
    std::uint32_t selected{0x6000u};
    std::int32_t rows{3};
    std::vector<std::uint32_t> grid{0u, 0x6100u, 0x6200u};

    std::uint32_t FirstSelectedItem(std::uint32_t list) override {
        Check(list == 0x5800u, "story selection list");
        calls.emplace_back("selected");
        return selected;
    }
    std::uint32_t ItemId(std::uint32_t item,
                         std::uint32_t offset) override {
        Check(offset == 0x138u, "story and mission item ID field");
        calls.emplace_back("id:" + std::to_string(item));
        return item == 0x6000u || item == 0x6200u ? 77u : 12u;
    }
    std::int32_t RowCount(std::uint32_t list) override {
        Check(list == 0x5400u, "mission row count target");
        calls.emplace_back("rows");
        return rows;
    }
    std::uint32_t GridItem(std::uint32_t list, std::uint32_t row,
                           std::uint32_t column) override {
        Check(list == 0x5400u && column == 0u && row < grid.size(),
              "mission grid reference");
        calls.emplace_back("grid:" + std::to_string(row));
        return grid[row];
    }
    float StoryScrollPosition(std::uint32_t list) override {
        Check(list == 0x5800u, "story scrollbar source");
        calls.emplace_back("scroll-position");
        return 37.25f;
    }
    void SetHoverRow(std::uint32_t list,
                     std::uint32_t row) override {
        Check(list == 0x5400u && row == 2u,
              "matching mission hover row");
        calls.emplace_back("hover");
    }
    void ScrollRowToVisible(std::uint32_t list,
                            std::uint32_t row) override {
        Check(list == 0x5400u && row == 2u,
              "matching mission row scroll");
        calls.emplace_back("scroll-row");
    }
    void SetStoryScrollPosition(std::uint32_t list,
                                float position) override {
        Check(list == 0x5800u && position == 37.25f,
              "story scrollbar restored");
        calls.emplace_back("restore");
    }
};

}  // namespace

int main() {
    pal4::MissionWindowState state{};
    state.Write(0x54u, 0x5400u);
    state.Write(0x58u, 0x5800u);
    Runtime runtime;
    runtime.selected = 0u;
    pal4::MissionWindowStorySelection::HandleClick(
        state, 0xdeadbeefu, runtime);
    Check(runtime.calls == std::vector<std::string>{"selected"},
          "no story selection exits immediately");

    runtime.selected = 0x6000u;
    runtime.rows = 0;
    runtime.calls.clear();
    pal4::MissionWindowStorySelection::HandleClick(state, 0u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "selected", "id:24576", "rows"},
          "empty mission list stops before scan");

    runtime.rows = 3;
    runtime.calls.clear();
    pal4::MissionWindowStorySelection::HandleClick(state, 0u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "selected", "id:24576", "rows", "grid:0", "grid:1",
              "id:24832", "grid:2", "id:25088", "scroll-position",
              "hover", "scroll-row", "restore"},
          "matching row updates hover and preserves story scroll");

    runtime.grid[2] = 0u;
    runtime.calls.clear();
    pal4::MissionWindowStorySelection::HandleClick(state, 0u, runtime);
    Check(runtime.calls.back() == "grid:2",
          "no match leaves both scroll positions untouched");
    std::cout << "Mission window story selection tests passed\n";
}
