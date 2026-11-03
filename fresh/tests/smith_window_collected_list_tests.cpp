#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowCollectedListRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t list = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x40u, list}, {object + 0x48u, 0x3000u},
        {object + 0x4cu, 0x3004u}, {object + 0x58u, 0x4000u},
        {object + 0x5cu, 0x4004u}, {object + 0x6cu, 0x4008u},
        {object + 0xbcu, 0x8000u}, {object + 0xc0u, 0x8008u},
        {0x8000u, 101u}, {0x8004u, 202u},
        {0x5000u + 0x9cu, 0x7000u},
        {0x5000u + 0x190u, 101u},
        {0x5000u + 0x168u, 0u}, {0x5000u + 0x16cu, 5u},
        {0x6000u + 0x9cu, 0x7004u},
        {0x6000u + 0x190u, 202u},
        {0x6000u + 0x94u, 2u},
        {0x6000u + 0x168u, 0u}, {0x6000u + 0x16cu, 0u},
        {0x6000u + 0x170u, 0u}, {0x6000u + 0x174u, 0u},
        {0xe000u + 0x138u, 101u},
        {0x5000u + 0xacu, 0x7008u},
        {0x5000u + 0xbcu, 0u},
    };
    std::vector<std::string> calls;
    std::vector<std::string> names;
    std::vector<bool> enabled;
    std::vector<std::string> numbers;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t>> cells;
    std::map<std::uint32_t, std::string> text;
    std::vector<std::uint32_t> hidden;
    std::uint32_t rows = 0u;
    std::uint32_t hover = 0xe000u;
    std::optional<std::string_view> category_name = "blade";
    std::string image;
    std::vector<std::string> converted;
    bool fail_name = false;

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
        Check(target == list && direction == 0u, "sort args");
        calls.emplace_back("sort");
    }
    void ResetList(std::uint32_t target) override {
        Check(target == list, "reset target");
        rows = 0u;
        calls.emplace_back("reset");
    }
    void AddRow(std::uint32_t target,
                std::uint32_t row_id) override {
        Check(target == list && row_id == 0u, "row args");
        ++rows;
        calls.emplace_back("add-row");
    }
    std::uint32_t RowCount(std::uint32_t target) override {
        Check(target == list, "row count target");
        return rows;
    }
    std::uint32_t FindInventoryItem(std::uint32_t id,
                                    std::uint32_t category) override {
        Check(category == 7u && (id == 101u || id == 202u),
              "inventory lookup args");
        calls.emplace_back("find");
        return id == 101u ? 0x5000u : 0x6000u;
    }
    bool CheckPlayerCondition(std::uint32_t item) override {
        Check(item == 0x5000u || item == 0x6000u,
              "condition item");
        calls.emplace_back("condition");
        return item == 0x5000u;
    }
    std::uint32_t CreateNameItem(std::uint32_t target,
                                  std::string_view name,
                                  bool condition) override {
        Check(target == list, "name item target");
        names.emplace_back(name);
        enabled.push_back(condition);
        calls.emplace_back("name-item");
        return fail_name ? 0u
            : 0x9000u + 0x1000u *
                static_cast<std::uint32_t>(names.size() - 1u);
    }
    std::uint32_t CreateNumberItem(std::uint32_t target,
                                    std::string_view number) override {
        Check(target == list, "number item target");
        numbers.emplace_back(number);
        calls.emplace_back("number-item");
        return 0xb000u + 0x1000u *
            static_cast<std::uint32_t>(numbers.size() - 1u);
    }
    void SetItem(std::uint32_t target, std::uint32_t item,
                 std::uint32_t column,
                 std::uint32_t row) override {
        Check(target == list, "cell list target");
        cells.emplace_back(item, column, row);
        calls.emplace_back("set-item");
    }
    std::optional<std::string_view> CategoryName(
        std::uint32_t category) override {
        Check(category == 2u, "last record category");
        calls.emplace_back("category-name");
        return category_name;
    }
    std::string_view Utf8Text(std::uint32_t source) override {
        calls.emplace_back("utf8");
        if (source == 0x7000u) return "sword";
        if (source == 0x7004u) return "bow";
        if (source == 0x7008u) return "chosen";
        Check(source == 0u, "utf8 source");
        return {};
    }
    void SetText(std::uint32_t window,
                 std::string_view value) override {
        text[window] = value;
        calls.emplace_back("text");
    }
    std::string_view DescribeItem(std::uint32_t item) override {
        Check(item == 0x5000u, "selected description item");
        calls.emplace_back("describe");
        return "description";
    }
    std::string_view ReadText(std::uint32_t source) override {
        Check(source == 0u, "extra description source");
        return {};
    }
    std::string Utf8Buffer(std::string_view value) override {
        converted.emplace_back(value);
        return std::string(value);
    }
    void SetImage(std::uint32_t window,
                  std::string_view imageset,
                  std::string_view name) override {
        Check(window == 0x4008u && imageset == "zi0",
              "category image args");
        image = name;
        calls.emplace_back("image");
    }
    void SetHoverRow(std::uint32_t target,
                     std::uint32_t row) override {
        Check(target == list && row == 0u, "hover row args");
        calls.emplace_back("hover-row");
    }
    std::uint32_t HoverItem(std::uint32_t target) override {
        Check(target == list, "hover list target");
        calls.emplace_back("hover-item");
        return hover;
    }
    void Call49F080(std::uint32_t owner) override {
        Check(owner == object, "49F080 owner");
        calls.emplace_back("49F080");
    }
    void Call49EF80(std::uint32_t owner) override {
        Check(owner == object, "49EF80 owner");
        calls.emplace_back("49EF80");
    }
    void SetVisible(std::uint32_t window,
                    bool visible) override {
        Check(!visible, "visibility state");
        hidden.push_back(window);
        calls.emplace_back("hide");
    }
};

void CheckRowsAndDisplay() {
    Runtime runtime;
    pal4::SmithWindow::RefreshCollectedEquipmentList(
        Runtime::object, runtime);
    Check(runtime.rows == 2u &&
              runtime.names == std::vector<std::string>{"sword", "bow"} &&
              runtime.enabled == std::vector<bool>{true, false} &&
              runtime.numbers == std::vector<std::string>{"5", "0"},
          "collected records build ordered rows");
    Check(runtime.words.at(0x9000u + 0x138u) == 101u &&
              runtime.words.at(0xa000u + 0x138u) == 202u &&
              runtime.cells == (std::vector<std::tuple<
                  std::uint32_t, std::uint32_t, std::uint32_t>>{
                      {0x9000u, 0u, 0u}, {0xb000u, 1u, 0u},
                      {0xa000u, 0u, 1u}, {0xc000u, 1u, 1u}}),
          "row cell IDs and positions");
    Check(runtime.text.at(0x4008u) == "blade" &&
              runtime.image == "qing" &&
              runtime.converted.front() == "blade" &&
              runtime.text.at(0x3000u) == "chosen" &&
              runtime.text.at(0x3004u) == "description" &&
              runtime.hidden == std::vector<std::uint32_t>{0x4000u, 0x4004u},
          "category and selected item display");
    Check(runtime.calls.front() == "read" &&
              runtime.calls.at(1) == "sort" &&
              runtime.calls.at(2) == "reset" &&
              runtime.calls.at(runtime.calls.size() - 5u) == "49EF80",
          "list reset and trailing state order");
}

void CheckAlternatePaths() {
    Runtime no_hover;
    no_hover.hover = 0u;
    no_hover.category_name.reset();
    pal4::SmithWindow::RefreshCollectedEquipmentList(
        Runtime::object, no_hover);
    Check(no_hover.text.at(0x4008u).empty() &&
              no_hover.text.at(0x3000u).empty() &&
              no_hover.text.at(0x3004u).empty(),
          "missing category and hover clear visible text");

    Runtime empty;
    empty.words[Runtime::object + 0xc0u] = 0x8000u;
    bool rejected = false;
    try {
        pal4::SmithWindow::RefreshCollectedEquipmentList(
            Runtime::object, empty);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    Check(rejected && empty.calls.at(2) == "reset" &&
              empty.names.empty(), "empty collected list precondition");

    Runtime failed;
    failed.fail_name = true;
    bool allocation_failed = false;
    try {
        pal4::SmithWindow::RefreshCollectedEquipmentList(
            Runtime::object, failed);
    } catch (const std::bad_alloc&) {
        allocation_failed = true;
    }
    Check(allocation_failed && failed.numbers.empty(),
          "required name-cell allocation failure");
}

}  // namespace

int main() {
    try {
        CheckRowsAndDisplay();
        CheckAlternatePaths();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
