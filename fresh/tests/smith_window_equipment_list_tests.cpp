#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowEquipmentListRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t list = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x40u, list}, {object + 0x48u, 0x4000u},
        {object + 0x4cu, 0x4100u},
        {object + 0x6cu, 0x3000u},
        {0x5000u + 0x9cu, 0x7000u},
        {0x5000u + 0x190u, 111u},
        {0x5000u + 0x168u, 0u},
        {0x5000u + 0x16cu, 12u},
        {0x6000u + 0x9cu, 0u},
        {0x6000u + 0x190u, 222u},
        {0x6000u + 0x168u, static_cast<std::uint32_t>(-4)},
        {0x6000u + 0x16cu, 0u},
        {0x6000u + 0x170u, 0u},
        {0x6000u + 0x174u, 0u},
        {0xa000u + 0x138u, 111u},
        {0xc000u + 0xacu, 0x7100u},
        {0xc000u + 0xbcu, 0x7200u},
    };
    std::vector<std::string> calls;
    std::vector<std::uint32_t> row_ids;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t>> cells;
    std::vector<std::string> numbers;
    std::vector<std::string> names;
    std::vector<bool> conditions;
    std::string image;
    std::string selected_name;
    std::string description;
    std::uint32_t hover_item = 0xa000u;
    std::int32_t hover_id = 0;
    std::int32_t item_count = 2;
    std::uint32_t release_count = 0u;
    bool fail_name = false;
    bool number_null = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void SetSortDirection(std::uint32_t target,
                          std::uint32_t direction) override {
        Check(target == list && direction == 0u, "sort arguments");
        calls.emplace_back("sort");
    }
    void ResetList(std::uint32_t target) override {
        Check(target == list, "reset target");
        calls.emplace_back("reset");
    }
    std::int32_t RightHoverItemId(std::uint32_t owner) override {
        Check(owner == object, "hover gate owner");
        calls.emplace_back("hover-id");
        return hover_id;
    }
    std::uint32_t CreateEmptyText() override {
        calls.emplace_back("create-text");
        return 0x3333u;
    }
    std::uint32_t ReleaseText(std::uint32_t handle) override {
        Check(handle == 0x3333u, "text release handle");
        ++release_count;
        calls.emplace_back("release-text");
        return 0xabcdu;
    }
    std::int32_t InventorySize(std::uint32_t category) override {
        Check(category < 9u, "inventory category");
        calls.emplace_back("size");
        return item_count;
    }
    std::uint32_t InventoryAt(std::uint32_t category,
                              std::uint32_t index) override {
        Check(category < 9u && index < 2u, "inventory index");
        calls.emplace_back("item");
        return index == 0u ? 0x5000u : 0x6000u;
    }
    void AddRow(std::uint32_t target, std::uint32_t id) override {
        Check(target == list, "add row target");
        row_ids.push_back(id);
        calls.emplace_back("add-row");
    }
    std::uint32_t RowCount(std::uint32_t target) override {
        Check(target == list, "row count target");
        calls.emplace_back("row-count");
        return static_cast<std::uint32_t>(row_ids.size());
    }
    std::string_view Utf8Text(std::uint32_t source) override {
        Check(source == 0x7000u || source == 0x7100u ||
                  source == 0u,
              "name source pointer");
        calls.emplace_back("utf8");
        if (source == 0u) return "";
        return source == 0x7000u ? "sword" : "helmet";
    }
    std::string_view DescribeItem(std::uint32_t item) override {
        Check(item == 0xc000u, "description record");
        calls.emplace_back("describe");
        return "base";
    }
    std::string_view ReadText(std::uint32_t source) override {
        Check(source == 0x7200u || source == 0u,
              "extra description source");
        calls.emplace_back("read-text");
        return source == 0u ? "" : "sharp";
    }
    std::string Utf8Buffer(std::string_view text) override {
        calls.emplace_back("utf8-buffer");
        return std::string(text);
    }
    bool CheckPlayerCondition(std::uint32_t item) override {
        Check(item == 0x5000u || item == 0x6000u,
              "condition item");
        calls.emplace_back("condition");
        return item == 0x5000u;
    }
    std::uint32_t CreateNameItem(std::uint32_t target,
                                  std::uint32_t empty_text,
                                  std::string_view name,
                                  bool enabled) override {
        Check(target == list && empty_text == 0x3333u,
              "name item constructor args");
        names.emplace_back(name);
        conditions.push_back(enabled);
        calls.emplace_back("name-item");
        if (fail_name) return 0u;
        return 0xa000u + static_cast<std::uint32_t>(names.size() - 1u)
            * 0x1000u;
    }
    std::uint32_t CreateNumberItem(std::uint32_t target,
                                    std::string_view number) override {
        Check(target == list, "number item target");
        numbers.emplace_back(number);
        calls.emplace_back("number-item");
        return number_null ? 0u
            : 0xd000u + static_cast<std::uint32_t>(numbers.size() - 1u)
                  * 0x1000u;
    }
    void SetItem(std::uint32_t target, std::uint32_t item,
                 std::uint32_t column,
                 std::uint32_t row) override {
        Check(target == list, "set item target");
        cells.emplace_back(item, column, row);
        calls.emplace_back("set-item");
    }
    void SetImage(std::uint32_t window,
                  std::string_view imageset,
                  std::string_view name) override {
        Check(window == 0x3000u && imageset == "zi0",
              "category image target");
        image = name;
        calls.emplace_back("image");
    }
    void SetHoverRow(std::uint32_t target,
                      std::uint32_t row) override {
        Check(target == list && row == 0u, "first hover row");
        calls.emplace_back("set-hover");
    }
    std::uint32_t HoverItem(std::uint32_t target) override {
        Check(target == list, "hover item target");
        calls.emplace_back("hover-item");
        return hover_item;
    }
    std::uint32_t FindInventoryItem(std::uint32_t id,
                                     std::uint32_t category) override {
        Check(id == 111u && category == 7u, "hover lookup id/category");
        calls.emplace_back("find-item");
        return 0xc000u;
    }
    void SetText(std::uint32_t window,
                 std::string_view text) override {
        Check(window == 0x4000u || window == 0x4100u,
              "selected item text window");
        if (window == 0x4000u)
            selected_name = text;
        else
            description = text;
        calls.emplace_back("set-text");
    }
    void Refresh(std::uint32_t owner) override {
        Check(owner == object, "refresh owner");
        calls.emplace_back("refresh");
    }
};

void CheckPopulatedList() {
    Runtime runtime;
    Check(pal4::SmithWindow::UpdateEquipmentList(
              Runtime::object, 3u, runtime) == 0xabcdu,
          "temporary text release result");
    Check(runtime.row_ids == (std::vector<std::uint32_t>{0u, 1u}) &&
              runtime.names == (std::vector<std::string>{"sword", ""}) &&
              runtime.conditions == (std::vector<bool>{true, false}) &&
              runtime.numbers == (std::vector<std::string>{"12", "0"}),
          "inventory rows, UTF-8 names, conditions and positive counts");
    Check(runtime.words.at(0xa000u + 0x138u) == 111u &&
              runtime.words.at(0xb000u + 0x138u) == 222u &&
              runtime.cells ==
                  (std::vector<std::tuple<std::uint32_t,
                                          std::uint32_t,
                                          std::uint32_t>>{
                      {0xa000u, 0u, 0u}, {0xd000u, 1u, 0u},
                      {0xb000u, 0u, 1u}, {0xe000u, 1u, 1u}}),
          "item IDs and two list columns");
    Check(runtime.image == "toubufanju" &&
              runtime.selected_name == "helmet" &&
              runtime.description ==
                  "base<br/><colour red=0 green=0 blue=0 alpha=255>(sharp)</colour>" &&
              runtime.release_count == 1u &&
              runtime.calls.back() == "release-text",
          "category icon, hover update and final cleanup");
}

void CheckCategoriesAndEmptyList() {
    constexpr std::array<std::string_view, 7> images{
        "jian", "shuangjian", "qing", "toubufanju",
        "shengtifangju", "zhubufangju", "peidaia"};
    for (std::uint32_t category = 0; category < images.size(); ++category) {
        Runtime runtime;
        runtime.item_count = 0;
        runtime.hover_item = 0u;
        (void)pal4::SmithWindow::UpdateEquipmentList(
            Runtime::object, category, runtime);
        Check(runtime.image == images[category] &&
                  runtime.row_ids.empty() &&
                  runtime.selected_name.empty() &&
                  runtime.description.empty() &&
                  runtime.release_count == 1u,
              "empty category updates icon and clears item display");
    }
    Runtime other;
    other.item_count = 0;
    other.hover_item = 0u;
    (void)pal4::SmithWindow::UpdateEquipmentList(
        Runtime::object, 7u, other);
    Check(other.image.empty() && other.release_count == 1u,
          "unsupported category does not set an icon");
}

void CheckSelectedName() {
    Runtime runtime;
    pal4::SmithWindow::UpdateEquipmentItemName(
        Runtime::object, 0xc000u, runtime);
    Check(runtime.selected_name == "helmet" &&
              runtime.calls == (std::vector<std::string>{
                  "read", "utf8", "read", "set-text"}),
          "selected record name converts to UTF-8");

    runtime.calls.clear();
    runtime.words[0xc000u + 0xacu] = 0u;
    pal4::SmithWindow::UpdateEquipmentItemName(
        Runtime::object, 0xc000u, runtime);
    Check(runtime.selected_name.empty() &&
              runtime.calls.size() == 4u,
          "null name source becomes empty text");

    runtime.calls.clear();
    pal4::SmithWindow::UpdateEquipmentItemName(
        Runtime::object, 0u, runtime);
    Check(runtime.selected_name.empty() &&
              runtime.calls == (std::vector<std::string>{
                  "read", "set-text"}),
          "null selected item clears text without conversion");
}

void CheckSelectedDescription() {
    Runtime runtime;
    pal4::SmithWindow::UpdateEquipmentItemDescription(
        Runtime::object, 0xc000u, runtime);
    Check(runtime.description ==
              "base<br/><colour red=0 green=0 blue=0 alpha=255>(sharp)</colour>" &&
              runtime.calls == (std::vector<std::string>{
                  "describe", "read", "read-text", "utf8-buffer",
                  "read", "set-text"}),
          "extra item text adds exact rich-text markup");

    runtime.calls.clear();
    runtime.words[0xc000u + 0xbcu] = 0u;
    pal4::SmithWindow::UpdateEquipmentItemDescription(
        Runtime::object, 0xc000u, runtime);
    Check(runtime.description == "base" &&
              runtime.calls.size() == 6u,
          "absent extra text preserves base description");

    runtime.calls.clear();
    pal4::SmithWindow::UpdateEquipmentItemDescription(
        Runtime::object, 0u, runtime);
    Check(runtime.description.empty() &&
              runtime.calls == (std::vector<std::string>{
                  "read", "set-text"}),
          "null item clears description without conversion");
}

void CheckGateAndFailures() {
    Runtime gated;
    gated.hover_id = -2;
    Check(pal4::SmithWindow::UpdateEquipmentList(
              Runtime::object, 1u, gated) == 0xfffffffeu &&
              gated.calls == (std::vector<std::string>{
                  "read", "sort", "reset", "hover-id"}) &&
              gated.release_count == 0u,
          "negative hover gate returns after list reset");

    Runtime failed;
    failed.fail_name = true;
    bool threw = false;
    try {
        (void)pal4::SmithWindow::UpdateEquipmentList(
            Runtime::object, 0u, failed);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && failed.release_count == 1u &&
              failed.calls.back() == "release-text",
          "first-cell allocation failure releases temporary text");

    Runtime null_number;
    null_number.item_count = 1;
    null_number.number_null = true;
    (void)pal4::SmithWindow::UpdateEquipmentList(
        Runtime::object, 0u, null_number);
    Check(std::get<0>(null_number.cells.at(1)) == 0u,
          "null second-cell allocation is forwarded to list");
}

}  // namespace

int main() {
    try {
        CheckPopulatedList();
        CheckCategoriesAndEmptyList();
        CheckGateAndFailures();
        CheckSelectedName();
        CheckSelectedDescription();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
