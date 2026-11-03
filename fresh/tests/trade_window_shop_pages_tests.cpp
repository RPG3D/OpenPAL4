#include "pal4/trade_window_shop_pages.h"
#include "pal4/trade_window_shop_page_navigation_runtime.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Cell {
    std::uint32_t item;
    std::uint32_t column;
    std::uint32_t row;
    bool flag_set_at_insert;
};

struct Created {
    std::uint32_t bytes;
    std::string text;
    bool special;
    bool condition;
};

struct Runtime final : pal4::ITradeWindowShopPageRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1028u, 0x2000u}, {0x10e4u, 0x3000u},
        {0x1050u, 0x3100u}, {0x1160u, 0x3200u},
        {0x9134u, 100u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> names{
        {0x7000u, "Sword"}, {0x7100u, "Armor"}};
    std::vector<std::uint32_t> inventory_items;
    std::vector<Created> created;
    std::vector<Cell> cells;
    std::vector<std::string> calls;
    std::uint32_t rows = 0u;
    std::uint32_t columns = 2u;
    std::uint32_t image_index = 0u;
    std::uint32_t image_window = 0u;
    std::uint32_t image = 0u;
    std::string named_image;
    bool correct_type = true;
    bool condition_two = true;
    bool condition = true;
    std::uint32_t seen_category = 0u;
    std::uint32_t fail_created = 0u;
    bool hover = false;
    bool cleared = false;
    unsigned condition_calls = 0u;
    unsigned cleanup_calls = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write-word");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("write-byte");
    }
    bool IsTradeList(std::uint32_t list) override {
        Check(list == 0x2000u, "type check list");
        calls.emplace_back("type");
        return correct_type;
    }
    std::uint32_t ImageIndex(std::uint32_t page) override {
        (void)page;
        calls.emplace_back("image-index");
        return image_index;
    }
    void SetImage(std::uint32_t window,
                  std::uint32_t value) override {
        image_window = window;
        image = value;
        calls.emplace_back("set-image");
    }
    void SetNamedImage(std::uint32_t window,
                       std::string_view imageset,
                       std::string_view name) override {
        Check(window == 0x3100u && imageset == "zi0",
              "category image target");
        named_image = name;
        calls.emplace_back("named-image");
    }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x2000u, "reset list");
        rows = 0u;
        calls.emplace_back("reset");
    }
    std::uint32_t ColumnCount(std::uint32_t list) override {
        Check(list == 0x2000u, "column count list");
        return columns;
    }
    void RemoveColumn(std::uint32_t list,
                      std::uint32_t column) override {
        Check(list == 0x2000u && column == 0u,
              "remove first column");
        calls.emplace_back("remove");
    }
    void AddColumn(std::uint32_t list, std::uint32_t column,
                   float width) override {
        Check(list == 0x2000u, "add column list");
        const float expected = column == 0u ? 0.44999999f :
            column == 1u ? 0.25f : 0.2f;
        Check(width == expected, "column width");
        calls.emplace_back("column:" + std::to_string(column));
    }
    void SetColumnLabelImage(std::uint32_t list,
                             std::uint32_t column,
                             std::string_view imageset,
                             std::string_view label) override {
        Check(list == 0x2000u && imageset == "zi0", "column image");
        const auto expected = column == 0u ? "mingcheng" :
            column == 1u ? "dengji" : "jiage";
        Check(label == expected, "column label");
        calls.emplace_back("label:" + std::to_string(column));
    }
    std::uint32_t InventorySize(std::uint32_t category) override {
        seen_category = category;
        return static_cast<std::uint32_t>(inventory_items.size());
    }
    std::uint32_t InventoryItem(std::uint32_t category,
                                std::uint32_t index) override {
        Check(category == seen_category, "inventory category");
        return inventory_items.at(index);
    }
    bool CheckPlayersConditionTwo(std::uint32_t item) override {
        Check(item != 0u, "condition two record");
        return condition_two;
    }
    bool CheckPlayersCondition(std::uint32_t item) override {
        Check(item != 0u, "condition record");
        ++condition_calls;
        return condition;
    }
    std::uint32_t InventoryBase() override { return 0x9000u; }
    std::string Utf8Name(std::uint32_t name) override {
        return name == 0u ? "" : names.at(name);
    }
    void AddRow(std::uint32_t list) override {
        Check(list == 0x2000u, "add row list");
        ++rows;
        calls.emplace_back("row");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x2000u, "row count list");
        return rows;
    }
    std::uint32_t CreateItem(std::uint32_t size,
                             std::uint32_t list,
                             std::string_view text,
                             bool special, bool ctor_condition) override {
        Check(list == 0x2000u, "create item list");
        created.push_back({size, std::string(text), special,
                           ctor_condition});
        if (created.size() == fail_created) return 0u;
        calls.emplace_back("create");
        return 0x5000u +
            static_cast<std::uint32_t>(created.size() - 1u) * 0x300u;
    }
    void SetItem(std::uint32_t list, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override {
        Check(list == 0x2000u, "set item list");
        cells.push_back({item, column, row,
                         bytes.contains(item + 0x1a5u)});
        calls.emplace_back("set-item");
    }
    void SetHoverRowIndex(std::uint32_t list,
                          std::int32_t row) override {
        Check(list == 0x2000u && row == 0, "first hover row");
        hover = true;
    }
    void ClearItemDisplay(std::uint32_t frame) override {
        Check(frame == 0x1000u, "clear display frame");
        cleared = true;
    }
    std::uint32_t CleanupTemporaryName() override {
        ++cleanup_calls;
        return 0x55u;
    }
};

struct NavigationRuntime final : pal4::ITradeWindowShopNavigationRuntime {
    explicit NavigationRuntime(Runtime& page_runtime) : pages(page_runtime) {}
    Runtime& pages;
    bool active = false;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return pages.ReadWord(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x10f8u, "navigation active flag");
        return active ? 1u : 0u;
    }
    void ShowActivePage(std::uint32_t, std::uint32_t) override {
        Check(false, "adapter must compose active page");
    }
    void ShowInactivePage(std::uint32_t, std::uint32_t) override {
        Check(false, "adapter must compose inactive page");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        return pages.RowCount(list);
    }
    std::uint32_t NextIndex(std::uint32_t,
                            std::uint32_t index) override {
        return (index + 1u) % 3u;
    }
    std::uint32_t PrevIndex(std::uint32_t,
                            std::uint32_t index) override {
        return (index + 2u) % 3u;
    }
    bool HasTradeItems(std::uint32_t) override { return true; }
    bool CanExecuteTrade(std::uint32_t) override { return true; }
    void SetVisible(std::uint32_t window, bool visible) override {
        visibility.emplace_back(window, visible);
    }
};

void AddRecord(Runtime& runtime, std::uint32_t record,
               std::uint32_t page, std::int32_t price,
               std::array<std::int32_t, 4> quantities) {
    runtime.words[record + 0x94u] = page;
    runtime.words[record + 0x90u] = 0xaau;
    runtime.words[record + 0x9cu] = 0x7000u;
    runtime.words[record + 0x178u] =
        static_cast<std::uint32_t>(price);
    runtime.words[record + 0x190u] = 0xbbu;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.words[record + 0x168u + 4u * i] =
            static_cast<std::uint32_t>(quantities[i]);
}

void CheckWrongWidget() {
    Runtime runtime;
    runtime.correct_type = false;
    Check(pal4::TradeWindowShopPages::UpdateInactive(
              0x1000u, 2u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"type"} &&
          runtime.cleanup_calls == 0u,
          "wrong widget exits before state updates");
    Check(pal4::TradeWindowShopPages::UpdateActive(
              0x1000u, 2u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{"type", "type"},
          "active page has same type gate");
}

void CheckInactiveRow() {
    Runtime runtime;
    runtime.image_index = 0u;
    runtime.inventory_items = {0x8000u};
    runtime.condition = false;
    AddRecord(runtime, 0x8000u, 2u, -9, {0, -2, 7, 3});
    Check(pal4::TradeWindowShopPages::UpdateInactive(
              0x1000u, 4u, runtime) == 0x55u &&
          runtime.words.at(0x10ecu) == 4u &&
          runtime.image_window == 0x3000u &&
          runtime.image == 0x3200u &&
          runtime.seen_category == 4u &&
          runtime.rows == 1u && runtime.hover &&
          runtime.named_image == "shengtifangju" &&
          runtime.cleanup_calls == 1u,
          "inactive page state, image and row");
    Check(runtime.created.size() == 3u &&
          runtime.created[0].bytes == 0x214u &&
          runtime.created[0].text == "Sword" &&
          runtime.created[0].special &&
          !runtime.created[0].condition &&
          runtime.created[1].text == "7" &&
          runtime.created[2].text == "-2" &&
          runtime.condition_calls == 2u,
          "inactive name constructor, quantity and signed quarter price");
    Check(runtime.cells.size() == 3u &&
          runtime.cells[0].column == 0u &&
          runtime.cells[1].column == 1u &&
          runtime.cells[2].column == 3u &&
          !runtime.cells[0].flag_set_at_insert &&
          runtime.words.at(0x5138u) == 0xbbu &&
          runtime.words.at(0x5438u) == 0xbbu &&
          runtime.words.at(0x5738u) == 0xaau &&
          runtime.bytes.at(0x51a5u) == 0u,
          "inactive row identifiers and post-insertion flag");
    const auto remove_count = std::count(runtime.calls.begin(),
                                         runtime.calls.end(), "remove");
    Check(remove_count == 2 &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "column:3") != runtime.calls.end(),
          "three columns use IDs zero, one and three");
}

void CheckInactiveEmpty() {
    Runtime runtime;
    runtime.image_index = 7u;
    runtime.inventory_items = {0x8000u};
    runtime.condition_two = false;
    AddRecord(runtime, 0x8000u, 1u, 8, {1, 0, 0, 0});
    (void)pal4::TradeWindowShopPages::UpdateInactive(
        0x1000u, 9u, runtime);
    Check(runtime.rows == 0u && runtime.created.empty() &&
          runtime.cleared && !runtime.hover &&
          runtime.image_window == 0u &&
          runtime.named_image == "xinzenga",
          "filtered empty list clears display and uses default icon");
}

void CheckTwoInactiveRows() {
    Runtime runtime;
    runtime.inventory_items = {0x8000u, 0x8200u};
    AddRecord(runtime, 0x8000u, 1u, 8, {1, 0, 0, 0});
    AddRecord(runtime, 0x8200u, 1u, 9, {-1, -2, -3, -4});
    runtime.words[0x829cu] = 0u;
    (void)pal4::TradeWindowShopPages::UpdateInactive(
        0x1000u, 1u, runtime);
    Check(runtime.rows == 2u && runtime.cells.size() == 6u &&
          runtime.cells[0].row == 0u &&
          runtime.cells[3].row == 1u &&
          runtime.cells[5].row == 1u &&
          runtime.created[3].text.empty() &&
          runtime.created[4].text == "-4" &&
          runtime.created[5].text == "2" && runtime.hover,
          "two rows preserve row indexes, empty name and last quantity");
}

void CheckActiveRows() {
    Runtime runtime;
    runtime.words[0x1134u] = 0x6000u;
    runtime.words[0x1138u] = 0x6008u;
    runtime.words[0x6000u] = 0x8000u;
    runtime.words[0x6004u] = 0x8200u;
    AddRecord(runtime, 0x8000u, 0u, 101, {0, 0, 0, -3});
    AddRecord(runtime, 0x8200u, 3u, 9, {2, 0, 0, 0});
    Check(pal4::TradeWindowShopPages::UpdateActive(
              0x1000u, 7u, runtime) == 0x55u &&
          runtime.words.at(0x10ecu) == 0u && runtime.rows == 1u &&
          runtime.created.size() == 3u &&
          runtime.created[0].bytes == 0x210u &&
          runtime.created[1].text == "-3" &&
          runtime.created[2].text == "101" &&
          runtime.named_image == "xinzenga" && runtime.hover,
          "active page seven stores zero, filters records and retains fourth quantity");
    Check(runtime.cells.size() == 3u &&
          runtime.cells[0].flag_set_at_insert &&
          runtime.cells[1].flag_set_at_insert &&
          runtime.cells[2].flag_set_at_insert &&
          runtime.words.at(0x5138u) == 0xaau &&
          runtime.words.at(0x5438u) == 0xaau &&
          runtime.words.at(0x5738u) == 0xaau &&
          runtime.bytes.at(0x51a5u) == 1u,
          "active price greater than money disables all three cells");
}

void CheckActiveEmptyAndSigned() {
    Runtime empty;
    empty.words[0x1134u] = 0u;
    empty.words[0x1138u] = 0u;
    (void)pal4::TradeWindowShopPages::UpdateActive(
        0x1000u, 2u, empty);
    Check(empty.cleared && empty.named_image == "qing" &&
          empty.rows == 0u, "empty active vector clears display");

    Runtime signed_money;
    signed_money.words[0x1134u] = 0x6000u;
    signed_money.words[0x1138u] = 0x6004u;
    signed_money.words[0x6000u] = 0x8000u;
    signed_money.words[0x9134u] =
        static_cast<std::uint32_t>(-5);
    AddRecord(signed_money, 0x8000u, 2u, -5, {1, 0, 0, 0});
    (void)pal4::TradeWindowShopPages::UpdateActive(
        0x1000u, 2u, signed_money);
    Check(signed_money.rows == 1u &&
          signed_money.bytes.at(0x51a5u) == 0u &&
          signed_money.named_image == "qing",
          "signed equal price and money leaves cells enabled");
}

void CheckItemAllocationFailure() {
    Runtime runtime;
    runtime.inventory_items = {0x8000u};
    runtime.fail_created = 2u;
    AddRecord(runtime, 0x8000u, 1u, 8, {1, 0, 0, 0});
    bool threw = false;
    try {
        (void)pal4::TradeWindowShopPages::UpdateInactive(
            0x1000u, 1u, runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.created.size() == 2u &&
          runtime.cells.size() == 1u,
          "failed item allocation throws after prior inserted cell");
}

void CheckAllCategoryImages() {
    constexpr std::array<const char*, 8> images{
        "jian", "shuangjian", "qing", "toubufanju",
        "shengtifangju", "zhubufangju", "peidaia", "xinzenga"};
    for (std::uint32_t category = 0u; category < images.size(); ++category) {
        Runtime runtime;
        runtime.columns = 0u;
        runtime.image_index = 5u;
        (void)pal4::TradeWindowShopPages::UpdateInactive(
            0x1000u, category, runtime);
        Check(runtime.named_image == images[category] &&
              runtime.image_window == 0u &&
              runtime.rows == 0u && runtime.cleared,
              "category image table and default branch");
    }
}

void CheckNavigationComposition() {
    Runtime pages;
    pages.words[0x1044u] = 0x3300u;
    pages.words[0x1048u] = 0x3400u;
    pages.inventory_items = {0x8000u};
    AddRecord(pages, 0x8000u, 2u, 8, {1, 0, 0, 0});
    NavigationRuntime navigation(pages);
    pal4::TradeWindowShopPageNavigationRuntime linked(navigation, pages);
    pal4::TradeWindowShopNavigation::UpdateShopData(
        0x1000u, 2u, true, linked);
    Check(pages.rows == 1u && pages.seen_category == 2u &&
          pages.words.at(0x10ecu) == 2u &&
          navigation.visibility ==
              std::vector<std::pair<std::uint32_t, bool>>{
                  {0x3300u, true}, {0x3400u, true}},
          "navigation composes inactive page and arrow visibility");

    Runtime active_pages;
    active_pages.words[0x1044u] = 0x3300u;
    active_pages.words[0x1048u] = 0x3400u;
    active_pages.words[0x1134u] = 0x6000u;
    active_pages.words[0x1138u] = 0x6004u;
    active_pages.words[0x6000u] = 0x8000u;
    AddRecord(active_pages, 0x8000u, 1u, 8, {1, 0, 0, 0});
    NavigationRuntime active_navigation(active_pages);
    active_navigation.active = true;
    pal4::TradeWindowShopPageNavigationRuntime active_link(
        active_navigation, active_pages);
    pal4::TradeWindowShopNavigation::UpdateShopData(
        0x1000u, 1u, false, active_link);
    Check(active_pages.rows == 1u &&
          active_pages.words.at(0x10ecu) == 1u &&
          active_pages.created.size() == 3u &&
          active_navigation.visibility.size() == 2u,
          "navigation composes active page");
}

}  // namespace

int main() {
    try {
        CheckWrongWidget();
        CheckInactiveRow();
        CheckInactiveEmpty();
        CheckTwoInactiveRows();
        CheckActiveRows();
        CheckActiveEmptyAndSigned();
        CheckItemAllocationFailure();
        CheckAllCategoryImages();
        CheckNavigationComposition();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
