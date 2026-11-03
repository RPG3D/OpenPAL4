#include "pal4/property_window_list.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using Record = pal4::PropertyInventoryRecord;

struct Runtime final : pal4::IPropertyWindowListRuntime {
    std::vector<std::string> calls;
    std::vector<Record> records;
    std::vector<std::string> item_text;
    std::uint32_t rows{};
    std::uint32_t selected_item{};
    std::uint32_t selected_row{};
    bool fail_recent{};
    bool fail_convert{};

    float ScrollPosition(std::uint32_t list) override {
        Check(list == 0x4000u, "scroll source is PropertyList");
        calls.emplace_back("scroll-position");
        return 0.375f;
    }
    std::uint32_t FirstSelectedItem(std::uint32_t list) override {
        Check(list == 0x4000u, "selection source is PropertyList");
        calls.emplace_back("first-selected");
        return selected_item;
    }
    std::uint32_t SelectedRow(std::uint32_t list,
                              std::uint32_t item) override {
        Check(list == 0x4000u && item == selected_item,
              "selected row resolves original list item");
        calls.emplace_back("selected-row");
        return selected_row;
    }
    void InitializeScratch() override {
        calls.emplace_back("scratch-init");
    }
    void CleanupScratch() override {
        calls.emplace_back("scratch-cleanup");
    }
    void SetItemPanel(pal4::PropertyWindowState& state,
                      bool visible) override {
        Check(state.Read(108u) != 0xffffffffu || visible,
              "category is stored before panel update");
        Check(visible, "item panel is enabled before list reset");
        calls.emplace_back("item-panel");
    }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x4000u, "reset target is PropertyList");
        calls.emplace_back("reset-list");
    }
    std::vector<std::uint32_t> RecentItemIds() override {
        calls.emplace_back("recent-copy");
        return {10u, 20u, 30u};
    }
    void ReleaseRecentItems() override {
        calls.emplace_back("recent-release");
    }
    std::optional<Record> RecentItem(std::uint32_t id) override {
        calls.push_back("recent:" + std::to_string(id));
        if (fail_recent) throw std::runtime_error("recent");
        if (id == 20u) return std::nullopt;
        return Record{0x3000u + id, id,
                      id == 30u ? 0xffffffffu : 2u,
                      id == 30u ? std::nullopt
                                : std::optional<std::string>{"Potion"}};
    }
    std::optional<Record> FeaturedItem(
        std::uint32_t category) override {
        calls.push_back("featured:" + std::to_string(category));
        if (category != 1u) return std::nullopt;
        return Record{0x3100u, 17u, 4u, "Featured"};
    }
    std::uint32_t CategoryItemCount(
        std::uint32_t category) override {
        calls.push_back("category-count:" + std::to_string(category));
        return category == 1u ? 3u : 0u;
    }
    std::optional<Record> CategoryItem(
        std::uint32_t category, std::uint32_t index) override {
        Check(category == 1u, "category index is forwarded");
        calls.push_back("category:" + std::to_string(index));
        if (index == 0u) return Record{0x3200u, 18u, 5u, "Attack"};
        if (index == 1u) return Record{0x3100u, 17u, 4u, "Featured"};
        return std::nullopt;
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.push_back("utf8:" + std::string(text));
        if (fail_convert) throw std::runtime_error("convert");
        return "utf8:" + std::string(text);
    }
    std::string FormatCount(std::int32_t count) override {
        calls.push_back("count:" + std::to_string(count));
        return std::to_string(count);
    }
    void AddRow(std::uint32_t list) override {
        Check(list == 0x4000u, "row target is PropertyList");
        ++rows;
        calls.emplace_back("add-row");
    }
    std::uint32_t CreateItem(std::uint32_t list,
                             std::string_view text,
                             std::uint32_t bytes,
                             std::uint32_t vtable) override {
        Check(list == 0x4000u && bytes == 0x210u &&
                  vtable == 0x8438d0u,
              "CEGUI item uses observed allocation and vtable");
        item_text.emplace_back(text);
        calls.push_back("create-item:" + std::string(text));
        return 0x5000u + static_cast<std::uint32_t>(item_text.size() * 4u);
    }
    void WriteItemId(std::uint32_t item,
                     std::uint32_t id) override {
        Check(item >= 0x5004u, "name item receives ID at +0x138");
        calls.push_back("id:" + std::to_string(id));
    }
    void SetItem(std::uint32_t list, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override {
        Check(list == 0x4000u && item >= 0x5004u &&
                  column <= 1u && row < rows,
              "item is placed in an existing two-column row");
        calls.push_back("set-item:" + std::to_string(column) + ":" +
                        std::to_string(row));
    }
    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        Check(window == 0x4100u && imageset == "zi0",
              "category image targets PropertyText using zi0");
        calls.push_back("image:" + std::string(image));
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x4000u, "row count uses PropertyList");
        calls.emplace_back("row-count");
        return rows;
    }
    void SetHoverRow(std::uint32_t list,
                     std::uint32_t row) override {
        Check(list == 0x4000u && row < rows,
              "hover target is a populated row");
        calls.push_back("hover:" + std::to_string(row));
    }
    void SetScrollPosition(std::uint32_t list,
                           float position) override {
        Check(list == 0x4000u && position == 0.375f,
              "original scrollbar position is restored");
        calls.emplace_back("restore-scroll");
    }
    void SetSelected(std::uint32_t button,
                     bool selected) override {
        calls.push_back("selected:" + std::to_string(button) +
                        (selected ? ":1" : ":0"));
    }
    void ClearDetails(pal4::PropertyWindowState&) override {
        calls.emplace_back("clear-details");
    }
};

pal4::PropertyWindowState MakeState() {
    pal4::PropertyWindowState state;
    state.Write(88u, 0x4000u);
    state.Write(92u, 0x4100u);
    for (std::uint32_t offset = 64u; offset <= 84u; offset += 4u)
        state.Write(offset, 0x4200u + offset);
    return state;
}

std::size_t Position(const std::vector<std::string>& calls,
                     std::string_view value) {
    const auto it = std::find(calls.begin(), calls.end(), value);
    Check(it != calls.end(), "expected operation is present");
    return static_cast<std::size_t>(it - calls.begin());
}

void TestRegularCategory() {
    auto state = MakeState();
    Runtime runtime;
    runtime.selected_item = 0x6200u;
    runtime.selected_row = 1u;
    pal4::PropertyWindowList::Populate(state, 1u, true, runtime);
    Check(state.Read(108u) == 1u && runtime.rows == 2u &&
              runtime.item_text == std::vector<std::string>{
                  "utf8:Featured", "utf8:4", "utf8:Attack", "utf8:5"},
          "featured item precedes distinct category item with two cells each");
    Check(Position(runtime.calls, "scroll-position") <
              Position(runtime.calls, "first-selected") &&
              Position(runtime.calls, "selected-row") <
              Position(runtime.calls, "reset-list") &&
              Position(runtime.calls, "featured:1") <
              Position(runtime.calls, "category-count:1") &&
              Position(runtime.calls, "category:2") <
              Position(runtime.calls, "image:gongjiwupin"),
          "selection and featured-item lookup precede category image");
    Check(Position(runtime.calls, "hover:1") <
              Position(runtime.calls, "restore-scroll") &&
              runtime.calls.back() == "scratch-cleanup" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "selected:16964:1") != runtime.calls.end(),
          "valid selected row, scrollbar and category button are restored");
}

void TestRecentCategory() {
    auto state = MakeState();
    Runtime runtime;
    runtime.selected_item = 0x6200u;
    runtime.selected_row = 7u;
    pal4::PropertyWindowList::Populate(state, 5u, true, runtime);
    Check(runtime.rows == 2u &&
              runtime.item_text == std::vector<std::string>{
                  "utf8:", "utf8:-1", "utf8:Potion", "utf8:2"},
          "recent IDs are traversed backward, missing record skipped and signed count formatted");
    Check(Position(runtime.calls, "recent:30") <
              Position(runtime.calls, "recent:20") &&
              Position(runtime.calls, "recent:20") <
              Position(runtime.calls, "recent:10") &&
              Position(runtime.calls, "recent-release") <
              Position(runtime.calls, "image:xinzenga"),
          "snapshot is released before common UI completion");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "hover:0") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "selected:16980:1") != runtime.calls.end(),
          "out-of-range selected row falls back to zero and selects recent button");
}

void TestEmptyAndFailure() {
    auto state = MakeState();
    Runtime runtime;
    pal4::PropertyWindowList::Populate(state, 0xffffffffu,
                                        false, runtime);
    Check(runtime.rows == 0u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "clear-details") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "restore-scroll") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "selected:16980:1") != runtime.calls.end(),
          "empty negative category clears details and uses last button");

    runtime = Runtime{};
    runtime.fail_recent = true;
    try {
        pal4::PropertyWindowList::Populate(state, 5u, false, runtime);
        Check(false, "recent lookup exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.size() >= 2u &&
                  runtime.calls[runtime.calls.size() - 2u] ==
                      "recent-release" &&
                  runtime.calls.back() == "scratch-cleanup",
              "recent snapshot and scratch are released on failure");
    }
    runtime = Runtime{};
    runtime.fail_convert = true;
    try {
        pal4::PropertyWindowList::Populate(state, 1u, false, runtime);
        Check(false, "conversion exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "scratch-cleanup",
              "scratch is released after item conversion failure");
    }
}

}  // namespace

int main() {
    TestRegularCategory();
    TestRecentCategory();
    TestEmptyAndFailure();
    std::cout << "Property window list tests passed\n";
}
