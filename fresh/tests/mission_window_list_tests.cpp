#include "pal4/mission_window_list.h"

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

struct Runtime final : pal4::IMissionWindowListRuntime {
    std::vector<std::string> calls;
    pal4::MissionRecordSlots slots;
    std::uint32_t columns{2u};
    std::uint32_t rows{};
    std::uint32_t created{};
    bool fail_item{};
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
        Check(list == 0x5400u, "mission list column target");
        calls.emplace_back("count");
        return columns;
    }
    void RemoveColumn(std::uint32_t list, std::uint32_t index) override {
        Check(list == 0x5400u && index == 0u && columns != 0u,
              "remove first mission column");
        --columns;
        calls.emplace_back("remove");
    }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x5400u && columns == 0u, "mission list reset");
        rows = 0u;
        calls.emplace_back("reset");
    }
    std::string TriggerTitle(std::uint32_t id) override {
        Check(id == 25u, "mission heading trigger");
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
              "single mission heading column");
        calls.emplace_back("column");
    }
    pal4::MissionRecordSlots LoadMissionRecords() override {
        calls.emplace_back("load");
        return slots;
    }
    void ReleaseMissionRecords() override {
        calls.emplace_back("release");
    }
    void AddRow(std::uint32_t list, std::uint32_t index) override {
        Check(list == 0x5400u && index == 0u, "append mission row");
        ++rows;
        calls.emplace_back("row");
    }
    std::uint32_t CreateItem(std::uint32_t list, std::string_view title,
                             std::uint32_t bytes,
                             std::uint32_t vtable) override {
        Check(list == 0x5400u && bytes == 0x210u &&
                  vtable == 0x8438d0u, "mission item layout");
        calls.emplace_back("item:" + std::string(title));
        if (fail_item) return 0u;
        return 0x1000u + ++created;
    }
    void WriteItemId(std::uint32_t item, std::uint32_t id) override {
        calls.emplace_back("id:" + std::to_string(item) + ":" +
                           std::to_string(id));
    }
    void WriteItemEnabled(std::uint32_t item, bool enabled) override {
        calls.emplace_back("enabled:" + std::to_string(item) + ":" +
                           std::to_string(enabled));
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x5400u, "mission row count target");
        calls.emplace_back("rows");
        return rows;
    }
    void SetItem(std::uint32_t list, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override {
        Check(list == 0x5400u && column == 0u && row + 1u == rows,
              "item placed in last row");
        calls.emplace_back("set:" + std::to_string(item) + ":" +
                           std::to_string(row));
    }
    void SetHoverRow(std::uint32_t list, std::int32_t row) override {
        Check(list == 0x5400u && row == 0, "first mission row hovered");
        calls.emplace_back("hover");
    }
    void SetMissionPanelVisibility(pal4::MissionWindowState& state,
                                   bool visible) override {
        Check(state.bytes[0x88u] == 1u && visible,
              "mission panel shown after rows");
        calls.emplace_back("panel");
        if (fail_panel) throw std::runtime_error("panel");
    }
};

pal4::MissionWindowState MakeState() {
    pal4::MissionWindowState state{};
    state.Write(0x54u, 0x5400u);
    state.Write(0x60u, 0x6000u);
    state.Write(0x64u, 0x6400u);
    state.Write(0x7cu, 0x7c00u);
    state.Write(0x80u, 0x8000u);
    return state;
}

void TestRecords() {
    auto state = MakeState();
    Runtime runtime;
    runtime.slots = {
        std::nullopt,
        pal4::MissionRecord{7u, 1, "first"},
        pal4::MissionRecord{9u, 0, "second"},
        pal4::MissionRecord{11u, -1, "third"},
    };
    pal4::MissionWindowList::Populate(state, runtime);
    const std::vector<std::string> expected{
        "image:32768:renwutishi", "image:31744:renwutishia",
        "selected:24576:1", "selected:25600:0",
        "count", "remove", "count", "remove", "count", "reset",
        "heading", "convert:legacy heading", "column", "load",
        "row", "convert:first", "item:UTF8:first", "id:4097:7",
        "enabled:4097:1", "rows", "set:4097:0",
        "row", "convert:second", "item:UTF8:second", "id:4098:9",
        "enabled:4098:0", "rows", "set:4098:1",
        "row", "convert:third", "item:UTF8:third", "id:4099:11",
        "enabled:4099:0", "rows", "set:4099:2",
        "rows", "hover", "panel", "release"};
    Check(runtime.calls == expected, "mission records and operation order");
    Check(runtime.rows == 3u && runtime.columns == 0u &&
              state.bytes[0x88u] == 1u,
          "list state after population");
}

void TestEmpty() {
    auto state = MakeState();
    Runtime runtime;
    runtime.columns = 0u;
    runtime.slots = {std::nullopt, std::nullopt};
    pal4::MissionWindowList::Populate(state, runtime);
    Check(runtime.rows == 0u &&
              runtime.calls[runtime.calls.size() - 3u] == "rows" &&
              runtime.calls[runtime.calls.size() - 2u] == "panel" &&
              runtime.calls.back() == "release",
          "empty list skips hover but shows panel and releases records");
}

void TestFailure(bool fail_item) {
    auto state = MakeState();
    Runtime runtime;
    runtime.columns = 0u;
    runtime.slots = {pal4::MissionRecord{5u, 1, "entry"}};
    runtime.fail_item = fail_item;
    runtime.fail_panel = !fail_item;
    try {
        pal4::MissionWindowList::Populate(state, runtime);
        Check(false, "failure must propagate");
    } catch (const std::exception&) {
        Check(runtime.calls.back() == "release",
              "database records released after failure");
    }
}

}  // namespace

int main() {
    TestRecords();
    TestEmpty();
    TestFailure(true);
    TestFailure(false);
    std::cout << "Mission window list tests passed\n";
}
