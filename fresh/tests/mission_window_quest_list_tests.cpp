#include "pal4/mission_window_quest_list.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMissionWindowQuestListRuntime {
    std::vector<std::string> calls;
    std::vector<pal4::MissionQuestRecord> records;
    std::uint32_t columns{2u};
    std::uint32_t quest_rows{};
    std::uint32_t story_rows{};
    std::uint32_t item_number{};
    std::int32_t limit{3};
    bool fail_rich{};
    bool fail_panel{};

    void SetImage(std::uint32_t image, std::string_view image_set,
                  std::string_view image_name) override {
        Check(image_set == "zi0", "image set");
        calls.emplace_back("image:" + std::to_string(image) + ":" +
                           std::string(image_name));
    }
    void SetSelected(std::uint32_t button, bool selected) override {
        calls.emplace_back("selected:" + std::to_string(button) + ":" +
                           std::to_string(selected));
    }
    std::uint32_t ColumnCount(std::uint32_t list) override {
        Check(list == 0x5400u, "quest column target");
        calls.emplace_back("count");
        return columns;
    }
    void RemoveColumn(std::uint32_t list, std::uint32_t index) override {
        Check(list == 0x5400u && index == 0u && columns != 0u,
              "remove first quest column");
        --columns;
        calls.emplace_back("remove");
    }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x5400u || list == 0x5800u, "reset list target");
        (list == 0x5400u ? quest_rows : story_rows) = 0u;
        calls.emplace_back("reset:" + std::to_string(list));
    }
    std::string TriggerTitle(std::uint32_t id) override {
        Check(id == 26u, "quest heading trigger");
        calls.emplace_back("heading");
        return "legacy heading";
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("convert:" + std::string(text));
        return "UTF8:" + std::string(text);
    }
    void AddColumn(std::uint32_t list, std::string_view title,
                   std::uint32_t id, float width) override {
        Check(list == 0x5400u && title == "UTF8:legacy heading" &&
                  id == 0u && width == 0.89999998f,
              "quest heading column");
        calls.emplace_back("column");
    }
    float AbsoluteColumnWidth(std::uint32_t list,
                              std::uint32_t index) override {
        Check(list == 0x5800u && index == 0u,
              "story column width conversion target");
        calls.emplace_back("width");
        return 482.5f;
    }
    std::int32_t StoryVisibilityLimit() override {
        calls.emplace_back("limit");
        return limit;
    }
    std::vector<pal4::MissionQuestRecord> LoadQuestRecords() override {
        calls.emplace_back("load");
        return records;
    }
    void ReleaseQuestRecords() override { calls.emplace_back("release"); }
    void AddRow(std::uint32_t list, std::uint32_t index) override {
        Check(index == 0u, "append row ID");
        ++(list == 0x5400u ? quest_rows : story_rows);
        calls.emplace_back("row:" + std::to_string(list));
    }
    std::uint32_t CreatePlainItem(std::uint32_t list,
                                  std::string_view title,
                                  std::uint32_t bytes,
                                  std::uint32_t vtable) override {
        Check(list == 0x5400u && bytes == 0x210u &&
                  vtable == 0x8438d0u, "plain item layout");
        calls.emplace_back("plain:" + std::string(title));
        return 0x1000u + ++item_number;
    }
    std::uint32_t CreateRichItem(std::string_view text, float width,
                                 std::uint32_t bytes) override {
        Check(width == 482.5f && bytes == 0x1b4u,
              "rich item width and allocation");
        calls.emplace_back("rich:" + std::string(text));
        if (fail_rich) return 0u;
        return 0x1000u + ++item_number;
    }
    void WriteItemId(std::uint32_t item, std::uint32_t id) override {
        calls.emplace_back("id:" + std::to_string(item) + ":" +
                           std::to_string(id));
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x5400u || list == 0x5800u,
              "row count target");
        calls.emplace_back("countrows:" + std::to_string(list));
        return list == 0x5400u ? quest_rows : story_rows;
    }
    void SetItem(std::uint32_t list, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override {
        Check(column == 0u &&
                  row + 1u == (list == 0x5400u ? quest_rows : story_rows),
              "item placed in last row");
        calls.emplace_back("set:" + std::to_string(list) + ":" +
                           std::to_string(item) + ":" +
                           std::to_string(row));
    }
    void SetHoverRow(std::uint32_t list, std::uint32_t row) override {
        Check(list == 0x5400u && row + 1u == quest_rows,
              "last quest row hovered");
        calls.emplace_back("hover:" + std::to_string(row));
    }
    void ScrollRowToVisible(std::uint32_t list,
                            std::uint32_t row) override {
        Check(list == 0x5400u && row + 1u == quest_rows,
              "last quest row scrolled");
        calls.emplace_back("scroll:" + std::to_string(row));
    }
    void SetMissionPanelVisibility(pal4::MissionWindowState& state,
                                   bool visible) override {
        Check(state.bytes[0x88u] == 0u && !visible,
              "quest panel selected");
        calls.emplace_back("panel");
        if (fail_panel) throw std::runtime_error("panel");
    }
};

pal4::MissionWindowState MakeState() {
    pal4::MissionWindowState state{};
    state.Write(0x54u, 0x5400u);
    state.Write(0x58u, 0x5800u);
    state.Write(0x60u, 0x6000u);
    state.Write(0x64u, 0x6400u);
    state.Write(0x7cu, 0x7c00u);
    state.Write(0x80u, 0x8000u);
    return state;
}

void TestFilteredRecords() {
    auto state = MakeState();
    Runtime runtime;
    runtime.records = {
        {-2, "early", "early description"},
        {3, "excluded", "excluded description"},
        {2, "latest", "latest description"},
    };
    pal4::MissionWindowQuestList::Populate(state, runtime);
    const std::vector<std::string> expected{
        "image:32768:juqingzhangjie", "image:31744:juqinggaishu",
        "selected:25600:1", "selected:24576:0",
        "count", "remove", "count", "remove", "count",
        "reset:21504", "heading", "convert:legacy heading", "column",
        "reset:22528", "width", "load", "limit", "limit", "limit",
        "row:21504", "convert:early", "plain:UTF8:early",
        "id:4097:4294967294", "countrows:21504", "set:21504:4097:0",
        "row:22528", "convert:early description",
        "rich:UTF8:early description", "id:4098:4294967294",
        "countrows:22528", "set:22528:4098:0",
        "row:21504", "convert:latest", "plain:UTF8:latest",
        "id:4099:2", "countrows:21504", "set:21504:4099:1",
        "row:22528", "convert:latest description",
        "rich:UTF8:latest description", "id:4100:2",
        "countrows:22528", "set:22528:4100:1",
        "countrows:21504", "countrows:21504", "hover:1",
        "scroll:1", "panel", "release"};
    Check(runtime.calls == expected, "filtered quest and story row sequence");
    Check(runtime.quest_rows == 2u && runtime.story_rows == 2u &&
              state.bytes[0x88u] == 0u,
          "paired list rows and selected mode");
}

void TestEmpty() {
    auto state = MakeState();
    Runtime runtime;
    runtime.columns = 0u;
    runtime.records = {{3, "excluded", "description"}};
    pal4::MissionWindowQuestList::Populate(state, runtime);
    Check(runtime.quest_rows == 0u && runtime.story_rows == 0u &&
              runtime.calls[runtime.calls.size() - 3u] ==
                  "countrows:21504" &&
              runtime.calls[runtime.calls.size() - 2u] == "panel" &&
              runtime.calls.back() == "release",
          "empty filtered list skips hover and scroll");
}

void TestFailure(bool fail_rich) {
    auto state = MakeState();
    Runtime runtime;
    runtime.columns = 0u;
    runtime.records = {{1, "title", "description"}};
    runtime.fail_rich = fail_rich;
    runtime.fail_panel = !fail_rich;
    try {
        pal4::MissionWindowQuestList::Populate(state, runtime);
        Check(false, "failure must propagate");
    } catch (const std::exception&) {
        Check(runtime.calls.back() == "release",
              "quest record array released on failure");
    }
}

}  // namespace

int main() {
    Check(pal4::MissionWindowQuestList::IsVisible(-1, 0) &&
              !pal4::MissionWindowQuestList::IsVisible(0, 0) &&
              pal4::MissionWindowQuestList::IsVisible(2, 3) &&
              !pal4::MissionWindowQuestList::IsVisible(3, 3),
          "signed quest filter compares record ID strictly below limit");
    TestFilteredRecords();
    TestEmpty();
    TestFailure(true);
    TestFailure(false);
    std::cout << "Mission window quest list tests passed\n";
}
