#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowSelectionClearRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t list = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x40u, list}, {object + 0x48u, 0x3000u},
        {object + 0x4cu, 0x3004u}, {object + 0x6cu, 0x3008u},
    };
    std::vector<std::string> calls;
    std::vector<std::uint32_t> removed;
    std::vector<std::tuple<std::uint32_t, float>> columns;
    std::vector<std::tuple<std::uint32_t, std::string,
                           std::string>> labels;
    std::map<std::uint32_t, std::string> text;
    std::uint32_t cleared_byte = 0u;
    bool initialized = false;
    std::int32_t column_count = 3;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == object + 0xacu && value == 0u,
              "selection byte");
        ++cleared_byte;
        calls.emplace_back("write-byte");
    }
    void SetSortDirection(std::uint32_t target,
                          std::uint32_t direction) override {
        Check(target == list && direction == 0u, "sort args");
        calls.emplace_back("sort");
    }
    void ResetList(std::uint32_t target) override {
        Check(target == list, "reset target");
        calls.emplace_back("reset");
    }
    bool ColumnsInitialized() override {
        calls.emplace_back("initialized?");
        return initialized;
    }
    void SetColumnsInitialized(bool value) override {
        Check(value, "initialized value");
        initialized = value;
        calls.emplace_back("initialized");
    }
    std::int32_t ColumnCount(std::uint32_t target) override {
        Check(target == list, "column count target");
        calls.emplace_back("column-count");
        return column_count;
    }
    void RemoveColumn(std::uint32_t target,
                      std::uint32_t column) override {
        Check(target == list, "remove column target");
        removed.push_back(column);
        calls.emplace_back("remove-column");
    }
    void SetUserColumnSizing(std::uint32_t target,
                             bool value) override {
        Check(target == list && !value, "column sizing");
        calls.emplace_back("sizing");
    }
    void SetUserColumnDragging(std::uint32_t target,
                               bool value) override {
        Check(target == list && !value, "column dragging");
        calls.emplace_back("dragging");
    }
    void SetUserSortControl(std::uint32_t target,
                            bool value) override {
        Check(target == list && !value, "sort control");
        calls.emplace_back("sort-control");
    }
    void AddColumn(std::uint32_t target,
                   std::uint32_t column, float width) override {
        Check(target == list, "add column target");
        columns.emplace_back(column, width);
        calls.emplace_back("add-column");
    }
    void SetColumnLabelImage(std::uint32_t target,
                             std::uint32_t column,
                             std::string_view imageset,
                             std::string_view image) override {
        Check(target == list, "column label target");
        labels.emplace_back(column, imageset, image);
        calls.emplace_back("label");
    }
    std::string_view Utf8Text(std::uint32_t) override {
        throw std::runtime_error("null item should skip UTF-8");
    }
    void SetText(std::uint32_t window,
                 std::string_view value) override {
        text[window] = value;
        calls.emplace_back("text");
    }
    std::string_view DescribeItem(std::uint32_t) override {
        throw std::runtime_error("null item should skip description");
    }
    std::string_view ReadText(std::uint32_t) override {
        throw std::runtime_error("null item should skip extra text");
    }
    std::string Utf8Buffer(std::string_view) override {
        throw std::runtime_error("null item should skip UTF-8 buffer");
    }
};

void CheckInitialLayout() {
    Runtime runtime;
    pal4::SmithWindow::ClearEquipmentSelection(
        Runtime::object, runtime);
    Check(runtime.removed ==
              std::vector<std::uint32_t>{0u, 0u, 0u} &&
              runtime.columns == (std::vector<
                  std::tuple<std::uint32_t, float>>{
                      {0u, 0.5f}, {1u, 0.395f}}) &&
              runtime.labels == (std::vector<std::tuple<
                  std::uint32_t, std::string, std::string>>{
                      {0u, "zi0", "mingcheng"},
                      {1u, "zi0", "dengji"}}),
          "first call configures both equipment columns");
    Check(runtime.initialized && runtime.cleared_byte == 1u &&
              runtime.text.size() == 3u &&
              runtime.text.at(0x3000u).empty() &&
              runtime.text.at(0x3004u).empty() &&
              runtime.text.at(0x3008u).empty(),
          "selection and all three text controls clear");
    Check(runtime.calls.at(0) == "read" &&
              runtime.calls.at(1) == "sort" &&
              runtime.calls.at(2) == "reset" &&
              runtime.calls.at(runtime.calls.size() - 3u) ==
                  "write-byte",
          "reset and selection-clear order");

    const auto before = runtime.calls.size();
    pal4::SmithWindow::ClearEquipmentSelection(
        Runtime::object, runtime);
    Check(runtime.calls.at(before + 3u) == "initialized?" &&
              runtime.removed.size() == 3u &&
              runtime.columns.size() == 2u &&
              runtime.cleared_byte == 2u,
          "repeat call skips one-time columns but clears display");
}

void CheckZeroColumns() {
    Runtime runtime;
    runtime.column_count = 0;
    pal4::SmithWindow::ClearEquipmentSelection(
        Runtime::object, runtime);
    Check(runtime.removed.empty() && runtime.columns.size() == 2u,
          "zero old columns still installs new layout");
}

}  // namespace

int main() {
    try {
        CheckInitialLayout();
        CheckZeroColumns();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
