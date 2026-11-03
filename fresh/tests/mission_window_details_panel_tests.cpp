#include "pal4/mission_window_details_panel.h"

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

struct Runtime final : pal4::IMissionWindowDetailsPanelRuntime {
    std::vector<std::string> calls;
    std::uint32_t hover{0x6000u};
    std::uint32_t mission{0x7000u};
    std::uint32_t chapter{0x8000u};
    std::int32_t rows{3};
    std::vector<std::uint32_t> grid{0u, 0x6100u, 0x6200u};

    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x5400u, "mission hover list");
        calls.emplace_back("hover-item");
        return hover;
    }
    std::uint32_t ItemId(std::uint32_t item,
                         std::uint32_t offset) override {
        Check(offset == 0x138u, "item ID offset");
        calls.emplace_back("item-id:" + std::to_string(item));
        if (item == 0x6000u) return 17u;
        return item == 0x6200u ? 77u : 12u;
    }
    std::uint32_t FindMission(std::uint32_t id) override {
        Check(id == 17u, "mission lookup ID");
        calls.emplace_back("mission");
        return mission;
    }
    std::uint32_t FindChapter(std::uint32_t id) override {
        Check(id == 17u, "chapter lookup ID");
        calls.emplace_back("chapter");
        return chapter;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x8400u && !visible, "mission mask hidden");
        calls.emplace_back("hide");
    }
    std::string QuestDescription(std::uint32_t quest,
                                  std::uint32_t offset) override {
        Check(quest == 0x7000u && offset == 0x38u,
              "mission description field");
        calls.emplace_back("description");
        return "details";
    }
    std::string ConvertToUtf8(std::string_view text) override {
        Check(text == "details", "description converted");
        calls.emplace_back("utf8");
        return "UTF8 details";
    }
    void SetText(std::uint32_t window,
                 std::string_view text) override {
        Check(window == 0x5c00u && text == "UTF8 details",
              "description text target");
        calls.emplace_back("text");
    }
    std::uint32_t LoadQuestImage(std::uint32_t quest) override {
        Check(quest == 0x7000u, "mission image source");
        calls.emplace_back("load-image");
        return 0x9000u;
    }
    void SetImage(std::uint32_t window,
                  std::uint32_t image) override {
        Check(window == 0x6800u && image == 0x9000u,
              "mission image target");
        calls.emplace_back("image");
    }
    std::uint32_t RecordId(std::uint32_t record) override {
        Check(record == 0x8000u, "chapter record pointer");
        calls.emplace_back("record-id");
        return 77u;
    }
    std::int32_t RowCount(std::uint32_t list) override {
        Check(list == 0x5800u, "story row count target");
        calls.emplace_back("rows");
        return rows;
    }
    std::uint32_t GridItem(std::uint32_t list, std::uint32_t row,
                           std::uint32_t column) override {
        Check(list == 0x5800u && column == 0u && row < grid.size(),
              "story grid reference");
        calls.emplace_back("grid:" + std::to_string(row));
        return grid[row];
    }
    void SetHoverRow(std::uint32_t list,
                     std::uint32_t row) override {
        Check(list == 0x5800u && row == 2u,
              "matching story row hovered");
        calls.emplace_back("hover-row");
    }
    void ScrollRowToTop(std::uint32_t list,
                        std::uint32_t row) override {
        Check(list == 0x5800u && row == 2u,
              "matching story row scrolled to top");
        calls.emplace_back("scroll-top");
    }
};

pal4::MissionWindowState MakeState() {
    pal4::MissionWindowState state{};
    state.Write(0x54u, 0x5400u);
    state.Write(0x58u, 0x5800u);
    state.Write(0x5cu, 0x5c00u);
    state.Write(0x68u, 0x6800u);
    state.Write(0x84u, 0x8400u);
    return state;
}

}  // namespace

int main() {
    auto state = MakeState();
    Runtime runtime;
    state.bytes[0x88u] = 1u;
    pal4::MissionWindowDetailsPanel::Update(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "hover-item", "item-id:24576", "mission", "hide",
              "description", "utf8", "text", "load-image", "image"},
          "mission mode composes hover lookup and detail update");
    runtime.calls.clear();
    runtime.mission = 0u;
    pal4::MissionWindowDetailsPanel::Update(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "hover-item", "item-id:24576", "mission"},
          "missing mission record leaves details unchanged");

    state.bytes[0x88u] = 0u;
    runtime.calls.clear();
    pal4::MissionWindowDetailsPanel::Update(state, runtime);
    const std::vector<std::string> chapter_calls{
              "hover-item", "item-id:24576", "chapter", "rows",
              "grid:0", "grid:1", "record-id", "item-id:24832",
              "grid:2", "record-id", "item-id:25088", "hover-row",
              "scroll-top"};
    Check(runtime.calls == chapter_calls,
          "chapter mode skips null grid cells and matches ID");
    runtime.calls.clear();
    runtime.grid[2] = 0u;
    pal4::MissionWindowDetailsPanel::Update(state, runtime);
    Check(runtime.calls.back() == "grid:2",
          "unmatched chapter leaves hover unchanged");
    runtime.calls.clear();
    runtime.rows = 0;
    pal4::MissionWindowDetailsPanel::Update(state, runtime);
    Check(runtime.calls.back() == "rows",
          "empty story list stops before scanning");
    runtime.calls.clear();
    runtime.chapter = 0u;
    pal4::MissionWindowDetailsPanel::Update(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "hover-item", "item-id:24576", "chapter"},
          "missing chapter record stops before row count");
    std::cout << "Mission window details panel tests passed\n";
}
