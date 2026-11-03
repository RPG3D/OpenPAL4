#include "pal4/trade_window_passive_page_navigation_runtime.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Cell {
    std::uint32_t column;
    std::uint32_t row;
    std::string text;
    std::uint32_t id;
    std::uint8_t disabled;
};

struct Runtime final : pal4::ITradeWindowPassivePageRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t list = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {frame + 0x28u, list}, {frame + 0xe4u, 0x3000u},
        {frame + 0x50u, 0x3100u}, {frame + 0x190u, 0x4000u},
        {frame + 0x194u, 0x4100u}, {frame + 0x198u, 0x4200u},
        {frame + 0x19cu, 0x4300u}, {frame + 0x154u, 0u},
        {frame + 0x158u, 0u}, {0x9134u, 100u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> names{
        {0x7000u, "Item"}, {0x7100u, "Effect"}};
    std::map<std::uint32_t, std::string> item_text;
    std::vector<Cell> cells;
    std::vector<std::pair<std::uint32_t, float>> columns;
    std::vector<std::pair<std::uint32_t, std::string>> labels;
    std::vector<std::string> calls;
    std::uint32_t rows = 0u;
    std::uint32_t old_columns = 2u;
    std::uint32_t next_item = 0x5000u;
    std::uint32_t fail_at = 0u;
    std::uint32_t create_count = 0u;
    std::uint32_t inventory_result = 0u;
    std::uint32_t found_item = 0xa000u;
    std::uint32_t found_effect = 0xb000u;
    std::uint32_t searched_db_id = 0u;
    std::uint32_t searched_inventory_id = 0u;
    std::uint32_t effect_input = 0u;
    std::uint32_t image = 0u;
    std::string bottom_image;
    bool active = true;
    bool correct_type = true;
    bool hover = false;
    bool cleared = false;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == frame + 0xf8u, "active gate offset");
        calls.emplace_back("active");
        return active ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write-word");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write-byte");
    }
    bool IsTradeList(std::uint32_t value) override {
        Check(value == list, "list type target");
        calls.emplace_back("type");
        return correct_type;
    }
    void SetImage(std::uint32_t window, std::uint32_t value) override {
        Check(window == 0x3000u, "tab image window");
        image = value;
        calls.emplace_back("image");
    }
    void SetNamedImage(std::uint32_t window,
                       std::string_view imageset,
                       std::string_view value) override {
        Check(window == 0x3100u && imageset == "zi0",
              "bottom image target");
        bottom_image = value;
        calls.emplace_back("bottom-image");
    }
    void ResetList(std::uint32_t value) override {
        Check(value == list, "reset target");
        rows = 0u;
        cells.clear();
        columns.clear();
        labels.clear();
        calls.emplace_back("reset");
    }
    std::uint32_t ColumnCount(std::uint32_t value) override {
        Check(value == list, "column count target");
        return old_columns;
    }
    void RemoveColumn(std::uint32_t value, std::uint32_t column) override {
        Check(value == list && column == 0u, "remove first column");
        calls.emplace_back("remove");
    }
    void AddColumn(std::uint32_t value, std::uint32_t column,
                   float width) override {
        Check(value == list, "add column target");
        columns.emplace_back(column, width);
    }
    void SetColumnLabelImage(std::uint32_t value,
                             std::uint32_t column,
                             std::string_view imageset,
                             std::string_view label) override {
        Check(value == list && imageset == "zi0", "column label target");
        labels.emplace_back(column, std::string(label));
    }
    std::uint32_t FindGameDbItem(std::uint32_t id) override {
        searched_db_id = id;
        calls.emplace_back("db");
        return id == 0u ? 0u : found_item;
    }
    std::uint32_t FindEffect(std::uint32_t item) override {
        effect_input = item;
        calls.emplace_back("effect");
        return found_effect;
    }
    std::uint32_t InventorySearch(std::uint32_t id) override {
        searched_inventory_id = id;
        calls.emplace_back("inventory");
        return inventory_result;
    }
    std::uint32_t InventoryBase() override { return 0x9000u; }
    std::string Utf8Name(std::uint32_t name) override {
        return name == 0u ? "" : names.at(name);
    }
    void AddRow(std::uint32_t value) override {
        Check(value == list, "add row target");
        ++rows;
        calls.emplace_back("row");
    }
    std::uint32_t RowCount(std::uint32_t value) override {
        Check(value == list, "row count target");
        return rows;
    }
    std::uint32_t CreateItem(std::uint32_t size,
                             std::uint32_t value,
                             std::string_view text) override {
        Check(size == 0x210u && value == list, "item allocation");
        ++create_count;
        calls.emplace_back("create");
        if (create_count == fail_at) return 0u;
        const auto item = next_item;
        next_item += 0x300u;
        item_text[item] = text;
        return item;
    }
    void SetItem(std::uint32_t value, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override {
        Check(value == list && words.contains(item + 0x138u) &&
                  bytes.contains(item + 0x1a5u),
              "cell fields written before insertion");
        cells.push_back({column, row, item_text.at(item),
                         words.at(item + 0x138u),
                         bytes.at(item + 0x1a5u)});
        calls.emplace_back("set-item");
    }
    void SetHoverRowIndex(std::uint32_t value,
                          std::int32_t row) override {
        Check(value == list && row == 0, "first hover row");
        hover = true;
        calls.emplace_back("hover");
    }
    void ClearItemDisplay(std::uint32_t value) override {
        Check(value == frame, "clear frame");
        cleared = true;
        calls.emplace_back("clear");
    }
};

void AddRecord(Runtime& runtime, std::uint32_t address,
               std::uint32_t page, std::uint32_t id,
               std::int32_t price) {
    runtime.words[address] = id;
    runtime.words[address + 8u] = 0x7000u;
    runtime.words[address + 0x64u] = page;
    runtime.words[address + 0x68u] = 0x42u;
    runtime.words[address + 0x6cu] = static_cast<std::uint32_t>(-12);
    runtime.words[address + 0x70u] = 34u;
    runtime.words[address + 0x78u] = static_cast<std::uint32_t>(price);
}

void SetRecords(Runtime& runtime, std::uint32_t count) {
    runtime.words[Runtime::frame + 0x154u] = 0xc000u;
    runtime.words[Runtime::frame + 0x158u] = 0xc000u + count * 4u;
}

void CheckGatesAndUnknownPage() {
    Runtime wrong;
    wrong.correct_type = false;
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 1u, wrong);
    Check(wrong.calls == std::vector<std::string>{"type"} &&
              !wrong.words.contains(Runtime::frame + 0xf4u),
          "wrong widget exits before active gate and mutation");

    Runtime inactive;
    inactive.active = false;
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 1u, inactive);
    Check(inactive.calls == std::vector<std::string>{"type", "active"},
          "inactive frame exits before page mutation");

    Runtime unknown;
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 3u, unknown);
    Check(unknown.words.at(Runtime::frame + 0xf4u) == 3u &&
              unknown.image == 0x4300u && unknown.columns.empty() &&
              unknown.rows == 0u && unknown.cleared &&
              unknown.bottom_image.empty() &&
              unknown.calls[2] == "write-word" &&
              unknown.calls[3] == "image" &&
              unknown.calls[4] == "reset" &&
              unknown.calls[5] == "remove" &&
              unknown.calls[6] == "remove",
          "unknown page still clears list and old columns");
}

void CheckObjectPage() {
    Runtime runtime;
    SetRecords(runtime, 4u);
    runtime.words[0xc000u] = 0xd000u;
    runtime.words[0xc004u] = 0xd100u;
    runtime.words[0xc008u] = 0xd200u;
    runtime.words[0xc00cu] = 0xd300u;
    AddRecord(runtime, 0xd000u, 1u, 1u, 1);
    AddRecord(runtime, 0xd100u, 0u, 0x55u, 120);
    AddRecord(runtime, 0xd200u, 0u, 0x66u, 40);
    AddRecord(runtime, 0xd300u, 0u, 0x77u, 40);
    runtime.words[0xd200u + 0x68u] = 0u;
    runtime.words[0xd300u + 0x68u] = 0x43u;
    runtime.found_item = 0xa000u;
    runtime.words[0xa000u + 0x168u] = 0u;
    runtime.words[0xa000u + 0x16cu] = 0u;
    runtime.words[0xa000u + 0x170u] = 7u;
    runtime.words[0xb004u] = 0x7100u;
    runtime.inventory_result = 0x100u;

    pal4::TradeWindowPassivePages::Update(Runtime::frame, 0u, runtime);
    Check(runtime.rows == 2u && runtime.cells.size() == 8u,
          "object page rows and four cells each");
    Check(runtime.columns ==
              (std::vector<std::pair<std::uint32_t, float>>{
                  {0u, 0.44999999f}, {1u, 0.15000001f},
                  {2u, 0.15000001f}, {3u, 0.15000001f}}) &&
              runtime.labels ==
              (std::vector<std::pair<std::uint32_t, std::string>>{
                  {0u, "mingcheng"}, {1u, "leibie"},
                  {2u, "dengji"}, {3u, "jiage"}}),
          "object page columns and labels");
    Check(runtime.cells[0].text == "Item" &&
              runtime.cells[1].text == "Effect" &&
              runtime.cells[2].text == "7" &&
              runtime.cells[3].text == "120" &&
              runtime.cells[0].id == 0x55u &&
              runtime.cells[0].disabled == 1u &&
              runtime.cells[4].row == 1u && runtime.hover &&
              runtime.bottom_image == "rongzhutupu",
          "object cell content, IDs, flags and final image");
    Check(runtime.searched_db_id == 0x43u &&
              runtime.effect_input == 0xa000u,
          "GameDB lookup uses record +0x68 and effect uses found item");
}

void CheckObjectMissingAndZeroQuantity() {
    Runtime missing;
    SetRecords(missing, 1u);
    missing.words[0xc000u] = 0xd000u;
    AddRecord(missing, 0xd000u, 0u, 0x55u, 20);
    missing.found_item = 0u;
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 0u, missing);
    Check(missing.rows == 0u && missing.cleared &&
              missing.searched_inventory_id == 0u,
          "missing GameDB record skips inventory and row");

    Runtime no_effect;
    SetRecords(no_effect, 1u);
    no_effect.words[0xc000u] = 0xd000u;
    AddRecord(no_effect, 0xd000u, 0u, 0x55u, 20);
    no_effect.found_effect = 0u;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        no_effect.words[0xa168u + 4u * i] = 0u;
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 0u, no_effect);
    Check(no_effect.rows == 1u && no_effect.cells.size() == 3u &&
              no_effect.cells[1].column == 2u &&
              no_effect.cells[1].text == "0" &&
              no_effect.cells[2].column == 3u,
          "missing effect leaves column one empty; all-zero quantity is zero");

    no_effect.words[0xa168u + 4u] = 0xfffffffeu;
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 0u, no_effect);
    Check(no_effect.cells[1].text == "-2",
          "first nonzero quantity uses signed decimal formatting");
}

void CheckRequirementPages() {
    for (std::uint32_t page = 1u; page <= 2u; ++page) {
        Runtime runtime;
        SetRecords(runtime, 2u);
        runtime.words[0xc000u] = 0xd000u;
        runtime.words[0xc004u] = 0xd100u;
        AddRecord(runtime, 0xd000u, page, 0x55u, -4);
        AddRecord(runtime, 0xd100u, 0u, 0x66u, 1);
        runtime.inventory_result = 0x100u;
        pal4::TradeWindowPassivePages::Update(Runtime::frame, page, runtime);
        Check(runtime.rows == 1u && runtime.cells.size() == 3u &&
                  runtime.cells[0].text == "Item" &&
                  runtime.cells[1].text ==
                      (page == 1u ? "-12" : "34") &&
                  runtime.cells[2].text == "-4" &&
                  runtime.cells[0].disabled == 0u &&
                  runtime.searched_inventory_id == 0x55u &&
                  runtime.searched_db_id == 0u,
              "page requirement offset, signed value and low-byte mask");
        Check(runtime.columns ==
                  (std::vector<std::pair<std::uint32_t, float>>{
                      {0u, 0.40000001f}, {1u, 0.30000001f},
                      {2u, 0.2f}}) &&
                  runtime.labels[1].second ==
                      (page == 1u ? "xuyaoqianli" : "xuyaolingxu") &&
                  runtime.bottom_image ==
                      (page == 1u ? "duanyetupu" : "zhulingtupu"),
              "page-specific columns and bottom image");
    }
}

void CheckInventoryFlagAndAllocation() {
    Runtime flagged;
    SetRecords(flagged, 1u);
    flagged.words[0xc000u] = 0xd000u;
    AddRecord(flagged, 0xd000u, 1u, 0x55u, 100);
    flagged.inventory_result = 0x101u;
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 1u, flagged);
    Check(flagged.cells[0].disabled == 1u &&
              flagged.cells[1].disabled == 1u &&
              flagged.cells[2].disabled == 1u,
          "nonzero low byte disables every cell at equal price");

    flagged.inventory_result = 0x180u;
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 1u, flagged);
    Check(flagged.cells[0].disabled == 0x80u,
          "inventory low byte is preserved rather than normalized");
    flagged.words[0xd000u + 0x78u] = 101u;
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 1u, flagged);
    Check(flagged.cells[0].disabled == 0x81u,
          "price comparison is ORed into inventory low byte");

    for (std::uint32_t fail = 1u; fail <= 3u; ++fail) {
        Runtime runtime;
        SetRecords(runtime, 1u);
        runtime.words[0xc000u] = 0xd000u;
        AddRecord(runtime, 0xd000u, 1u, 0x55u, 20);
        runtime.fail_at = fail;
        bool threw = false;
        try {
            pal4::TradeWindowPassivePages::Update(
                Runtime::frame, 1u, runtime);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        Check(threw && runtime.create_count == fail &&
                  runtime.cells.size() == fail - 1u &&
                  runtime.bottom_image.empty(),
              "allocation failure stops before inserting null item");
    }
}

struct Navigation final : pal4::ITradeWindowPassiveNavigationRuntime {
    Runtime& pages;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    explicit Navigation(Runtime& value) : pages(value) {}
    std::uint8_t ReadByte(std::uint32_t address) override {
        return pages.ReadByte(address);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return pages.ReadWord(address);
    }
    void ShowActivePage(std::uint32_t, std::uint32_t) override {
        Check(false, "adapter must render page");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        return pages.RowCount(list);
    }
    std::uint32_t NextIndex(std::uint32_t,
                            std::uint32_t index) override {
        (void)index;
        Check(false, "adapter must compose next index");
        return 0u;
    }
    std::uint32_t PreviousIndex(std::uint32_t,
                                std::uint32_t index) override {
        (void)index;
        Check(false, "adapter must compose previous index");
        return 0u;
    }
    bool HasTradeItems(std::uint32_t) override { return true; }
    bool CanExecuteTrade(std::uint32_t) override { return true; }
    void SetVisible(std::uint32_t window, bool value) override {
        visibility.emplace_back(window, value);
    }
};

void CheckNavigationComposition() {
    Runtime runtime;
    SetRecords(runtime, 1u);
    runtime.words[0xc000u] = 0xd000u;
    AddRecord(runtime, 0xd000u, 2u, 0x55u, 20);
    runtime.words[Runtime::frame + 0x44u] = 0x6000u;
    runtime.words[Runtime::frame + 0x48u] = 0x6100u;
    Navigation navigation(runtime);
    pal4::TradeWindowPassivePageNavigationRuntime adapter(
        navigation, runtime);
    pal4::TradeWindowPassiveNavigation::Update(
        Runtime::frame, 0u, true, adapter);
    Check(runtime.words.at(Runtime::frame + 0xf4u) == 2u &&
              runtime.rows == 1u && runtime.cells.size() == 3u &&
              runtime.bottom_image == "zhulingtupu" &&
              navigation.visibility ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0x6000u, true}, {0x6100u, true}}),
          "navigation visits empty pages and renders page two");
}

}  // namespace

int main() {
    try {
        CheckGatesAndUnknownPage();
        CheckObjectPage();
        CheckObjectMissingAndZeroQuantity();
        CheckRequirementPages();
        CheckInventoryFlagAndAllocation();
        CheckNavigationComposition();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
