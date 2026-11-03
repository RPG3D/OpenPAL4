#include "pal4/trade_window_character_pages.h"
#include "pal4/trade_window_character_page_navigation_runtime.h"
#include "pal4/trade_window_character_page_display_runtime.h"

#include <algorithm>
#include <array>
#include <bit>
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
    std::uint32_t item;
    std::uint32_t id;
    std::string text;
};

struct Runtime final : pal4::ITradeWindowCharacterPageRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t list = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {frame + 0x28u, list}, {frame + 0xe4u, 0x3000u},
        {frame + 0x50u, 0x3100u}, {frame + 0x178u, 0x4000u},
        {frame + 0x17cu, 0x4100u}, {frame + 0x180u, 0x4200u},
        {frame + 0x184u, 0x4300u}, {frame + 0x188u, 0x4400u},
        {frame + 0x144u, 0u}, {frame + 0x148u, 0u},
        {0x9134u, 100u}};
    std::vector<std::uint32_t> elements;
    std::vector<Cell> cells;
    std::vector<std::pair<std::uint32_t, float>> columns;
    std::vector<std::pair<std::uint32_t, std::string>> labels;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::string> item_text;
    std::map<std::uint32_t, std::int32_t> available_by_id;
    std::map<std::uint32_t, std::uint32_t> inventory_by_id;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> names{
        {0x7000u, "Herb"}, {0x7100u, "Sword"}};
    std::uint32_t rows = 0u;
    std::uint32_t old_columns = 2u;
    std::uint32_t next_item = 0x5000u;
    std::uint32_t allocation_count = 0u;
    std::uint32_t fail_at = 0u;
    std::uint32_t seen_page = 0u;
    std::uint32_t image = 0u;
    std::string bottom_image;
    bool correct_type = true;
    bool hover = false;
    bool cleared = false;
    unsigned cleanup_calls = 0u;
    std::int32_t size_override = -1;
    std::uint32_t only_page = 0xffffffffu;

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
                       std::string_view name) override {
        Check(window == 0x3100u && imageset == "zi0",
              "bottom image window");
        bottom_image = name;
        calls.emplace_back("bottom-image");
    }
    void ResetList(std::uint32_t value) override {
        Check(value == list, "reset list");
        rows = 0u;
        cells.clear();
        columns.clear();
        labels.clear();
        calls.emplace_back("reset");
    }
    std::uint32_t ColumnCount(std::uint32_t value) override {
        Check(value == list, "column count list");
        return old_columns;
    }
    void RemoveColumn(std::uint32_t value,
                      std::uint32_t column) override {
        Check(value == list && column == 0u, "remove first column");
        calls.emplace_back("remove");
    }
    void AddColumn(std::uint32_t value, std::uint32_t column,
                   float width) override {
        Check(value == list, "column list");
        columns.emplace_back(column, width);
        calls.emplace_back("column");
    }
    void SetColumnLabelImage(std::uint32_t value,
                             std::uint32_t column,
                             std::string_view imageset,
                             std::string_view label) override {
        Check(value == list && imageset == "zi0", "column label list");
        labels.emplace_back(column, std::string(label));
        calls.emplace_back("label");
    }
    std::int32_t InventorySize(std::uint32_t page) override {
        seen_page = page;
        calls.emplace_back("size");
        if (only_page != 0xffffffffu && page != only_page) return 0;
        return size_override >= 0 ? size_override :
            static_cast<std::int32_t>(elements.size());
    }
    std::uint32_t InventoryElement(std::uint32_t page,
                                   std::uint32_t reverse_index) override {
        Check(page == seen_page, "inventory page");
        calls.emplace_back("element:" + std::to_string(reverse_index));
        return elements.at(reverse_index);
    }
    std::uint32_t FindInventoryElement(std::uint32_t id,
                                       std::uint32_t page) override {
        Check(page == words.at(frame + 0xf0u),
              "active inventory category");
        calls.emplace_back("find-inventory");
        return inventory_by_id.contains(id) ? inventory_by_id.at(id) : 0u;
    }
    std::uint32_t InventoryBase() override { return 0x9000u; }
    std::string Utf8Name(std::uint32_t name) override {
        calls.emplace_back("utf8-name");
        return name == 0u ? "" : names.at(name);
    }
    void AddRow(std::uint32_t value) override {
        Check(value == list, "add row list");
        ++rows;
        calls.emplace_back("row");
    }
    std::uint32_t RowCount(std::uint32_t value) override {
        Check(value == list, "row count list");
        return rows;
    }
    std::uint32_t CreateTextItem(std::uint32_t bytes,
                                 std::uint32_t value,
                                 std::string_view text) override {
        Check(bytes == 0x210u && value == list, "text cell allocation");
        ++allocation_count;
        calls.emplace_back("create-text");
        if (allocation_count == fail_at) return 0u;
        const auto item = next_item;
        next_item += 0x300u;
        item_text[item] = text;
        return item;
    }
    std::uint32_t CreateSpinnerItem(std::uint32_t bytes) override {
        Check(bytes == 0x1b4u, "spinner cell allocation");
        ++allocation_count;
        calls.emplace_back("create-spinner");
        if (allocation_count == fail_at) return 0u;
        const auto item = next_item;
        next_item += 0x300u;
        item_text[item] = "";
        return item;
    }
    void SetItem(std::uint32_t value, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override {
        Check(value == list && words.contains(item + 0x138u),
              "cell ID before insertion");
        if (active_cells)
            Check(bytes.contains(item + 0x1a5u),
                  "active disabled byte before insertion");
        cells.push_back({column, row, item, words.at(item + 0x138u),
                         item_text.at(item)});
        calls.emplace_back("set-item");
    }
    void SetSpinnerRange(std::uint32_t item,
                         std::int32_t minimum,
                         std::int32_t maximum) override {
        Check(cells.back().item == item && cells.back().column == 1u,
              "spinner range called after insertion");
        Check(minimum == 0, "spinner range minimum");
        const auto expected = active_cells
            ? std::bit_cast<std::int32_t>(
                  99u - (inventory_by_id.contains(cells.back().id)
                      ? words.at(inventory_by_id.at(cells.back().id) + 8u)
                      : 0u))
            : available_by_id.at(cells.back().id);
        Check(maximum == expected,
              "spinner range maximum");
        calls.emplace_back("range");
    }
    void BindSpinnerState(std::uint32_t item) override {
        Check(words.at(item + 0x134u) == cells.back().row,
              "spinner row before state binding");
        calls.emplace_back("bind");
    }
    void SetHoverRowIndex(std::uint32_t value,
                          std::int32_t row) override {
        Check(value == list && row == 0, "first hover row");
        hover = true;
        calls.emplace_back("hover");
    }
    void ClearItemDisplay(std::uint32_t owner) override {
        Check(owner == frame, "clear item frame");
        cleared = true;
        calls.emplace_back("clear");
    }
    std::uint32_t CleanupPageBuffers() override {
        ++cleanup_calls;
        calls.emplace_back("cleanup");
        return 0x55u;
    }
    bool active_cells = false;
};

void AddElement(Runtime& runtime, std::uint32_t element,
                std::uint32_t record, std::uint32_t id,
                std::int32_t available, std::uint32_t name,
                std::int32_t base_price, bool visible) {
    runtime.elements.push_back(element);
    runtime.words[element] = record;
    runtime.words[element + 4u] = id;
    runtime.available_by_id[id] = available;
    runtime.words[element + 8u] =
        static_cast<std::uint32_t>(available);
    runtime.words[record + 0x14u] = name;
    runtime.words[record + 0x90u] =
        static_cast<std::uint32_t>(base_price);
    runtime.words[record + 0xa0u] = visible ? 1u : 0u;
}

void AddActiveRecord(Runtime& runtime, std::uint32_t slot,
                     std::uint32_t record, std::uint32_t id,
                     std::uint32_t page, std::uint32_t name,
                     std::int32_t price) {
    runtime.words[slot] = record;
    runtime.words[record] = id;
    runtime.words[record + 4u] = page;
    runtime.words[record + 0x14u] = name;
    runtime.words[record + 0x90u] =
        static_cast<std::uint32_t>(price);
}

void CheckGateAndEmptyPages() {
    Runtime wrong;
    wrong.correct_type = false;
    Check(pal4::TradeWindowCharacterPages::Update(
              Runtime::frame, 1u, wrong) == 1u &&
              wrong.calls == std::vector<std::string>{"type"} &&
              wrong.cleanup_calls == 0u,
          "wrong widget returns before buffers and page state");

    Runtime empty;
    Check(pal4::TradeWindowCharacterPages::Update(
              Runtime::frame, 0u, empty) == 0x55u &&
              empty.image == 0x4000u && empty.cleared &&
              !empty.hover && empty.bottom_image == "huifu" &&
              empty.columns ==
                  (std::vector<std::pair<std::uint32_t, float>>{
                      {0u, 0.38f}, {1u, 0.2f},
                      {2u, 0.15000001f}, {3u, 0.17f}}) &&
              empty.labels ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0u, "mingcheng"}, {1u, "shuliang"},
                      {2u, "cunliang"}, {3u, "jiage"}}),
          "empty page resets columns and display");
    Check(empty.calls[0] == "type" &&
              empty.calls[1] == "write-word" &&
              empty.calls[2] == "image" &&
              empty.calls[3] == "reset" &&
              empty.calls[4] == "remove" &&
              empty.calls[5] == "remove" &&
              empty.calls.back() == "cleanup",
          "page initialization and cleanup call order");

    constexpr std::array<const char*, 5> images{
        "huifu", "gongjiwupin", "fuzhuwuping", "cailiao", "juqing"};
    for (std::uint32_t page = 0u; page < 5u; ++page) {
        Runtime runtime;
        (void)pal4::TradeWindowCharacterPages::Update(
            Runtime::frame, page, runtime);
        Check(runtime.words.at(Runtime::frame + 0xf0u) == page &&
                  runtime.image == 0x4000u + page * 0x100u &&
                  runtime.bottom_image == images[page] &&
                  runtime.seen_page == page,
              "five page images and inventory category");
    }

    Runtime other;
    (void)pal4::TradeWindowCharacterPages::Update(
        Runtime::frame, 5u, other);
    Check(other.image == 0u && other.bottom_image == "huifu" &&
              other.seen_page == 5u && other.cleared,
          "out-of-range page still rebuilds list with fallback image");
    Runtime negative;
    (void)pal4::TradeWindowCharacterPages::Update(
        Runtime::frame, 0xffffffffu, negative);
    Check(negative.image == 0u && negative.bottom_image == "huifu",
          "signed negative page skips tab image");
}

void CheckRowsAndSpinner() {
    Runtime runtime;
    AddElement(runtime, 0xc000u, 0xd000u, 11u, 4, 0x7000u,
               101, true);
    AddElement(runtime, 0xc100u, 0xd100u, 12u, 9, 0x7100u,
               40, false);
    AddElement(runtime, 0xc200u, 0xd200u, 13u, -2, 0x7100u,
               -7, true);
    Check(pal4::TradeWindowCharacterPages::Update(
              Runtime::frame, 2u, runtime) == 0x55u &&
              runtime.rows == 2u && runtime.cells.size() == 8u &&
              runtime.hover && !runtime.cleared,
          "visible inventory records add four cells each");
    Check(runtime.cells[0].column == 0u &&
              runtime.cells[0].text == "Herb" &&
              runtime.cells[0].id == 11u &&
              runtime.cells[1].column == 1u &&
              runtime.cells[1].text.empty() &&
              runtime.cells[2].column == 2u &&
              runtime.cells[2].text == "4" &&
              runtime.cells[3].column == 3u &&
              runtime.cells[3].text == "25",
          "first row name, spinner, stock and quarter price");
    Check(runtime.cells[4].row == 1u &&
              runtime.cells[4].id == 13u &&
              runtime.cells[6].text == "-2" &&
              runtime.cells[7].text == "-1" &&
              runtime.words.at(runtime.cells[5].item + 0x134u) == 1u &&
              runtime.bottom_image == "fuzhuwuping",
          "second row signed values, spinner row and bottom image");
    Check(runtime.calls.end() != std::find(runtime.calls.begin(),
                                           runtime.calls.end(),
                                           "element:2") &&
              runtime.allocation_count == 8u,
          "reverse inventory indices include filtered entries");
}

void CheckAllocationFailure() {
    for (std::uint32_t fail = 1u; fail <= 4u; ++fail) {
        Runtime runtime;
        AddElement(runtime, 0xc000u, 0xd000u, 11u, 3, 0x7000u,
                   12, true);
        runtime.fail_at = fail;
        bool threw = false;
        try {
            (void)pal4::TradeWindowCharacterPages::Update(
                Runtime::frame, 0u, runtime);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        Check(threw && runtime.allocation_count == fail &&
                  runtime.cells.size() == fail - 1u &&
                  runtime.cleanup_calls == 1u &&
                  runtime.bottom_image.empty(),
              "allocation failure unwinds before null insertion");
    }
}

void CheckActivePages() {
    Runtime runtime;
    runtime.active_cells = true;
    runtime.words[Runtime::frame + 0x144u] = 0xc000u;
    runtime.words[Runtime::frame + 0x148u] = 0xc00cu;
    AddActiveRecord(runtime, 0xc000u, 0xd000u, 10u,
                    0u, 0x7000u, 40);
    AddActiveRecord(runtime, 0xc004u, 0xd100u, 11u,
                    1u, 0x7000u, 120);
    AddActiveRecord(runtime, 0xc008u, 0xd200u, 13u,
                    1u, 0x7100u, -7);
    runtime.inventory_by_id[11u] = 0xe000u;
    runtime.words[0xe008u] = 5u;
    Check(pal4::TradeWindowCharacterPages::UpdateActive(
              Runtime::frame, 1u, runtime) == 0x55u &&
              runtime.rows == 2u && runtime.cells.size() == 8u &&
              runtime.image == 0x4100u &&
              runtime.bottom_image == "gongjiwupin" &&
              runtime.hover,
          "active page filters transaction category and adds two rows");
    Check(runtime.cells[0].id == 11u &&
              runtime.cells[0].text == "Herb" &&
              runtime.cells[1].column == 1u &&
              runtime.cells[2].text == "5" &&
              runtime.cells[3].text == "120" &&
              runtime.cells[4].id == 13u &&
              runtime.cells[6].text == "0" &&
              runtime.cells[7].text == "-7",
          "active row name, owned stock, raw signed price and missing stock");
    for (std::size_t i = 0u; i < runtime.cells.size(); ++i) {
        const auto expected = i < 4u ? 1u : 0u;
        Check(runtime.bytes.at(runtime.cells[i].item + 0x1a5u) == expected,
              "every active cell carries price affordability byte");
    }
    Check(runtime.words.at(runtime.cells[1].item + 0x134u) == 0u &&
              runtime.words.at(runtime.cells[5].item + 0x134u) == 1u &&
              runtime.allocation_count == 8u,
          "active spinners preserve row indices and item sizes");

    Runtime page_zero;
    page_zero.active_cells = true;
    (void)pal4::TradeWindowCharacterPages::UpdateActive(
        Runtime::frame, 0u, page_zero);
    Check(page_zero.image == 0u && page_zero.cleared &&
              page_zero.bottom_image == "huifu",
          "active page zero intentionally keeps current tab image");
}

void CheckActiveAllocationFailure() {
    for (std::uint32_t fail = 1u; fail <= 4u; ++fail) {
        Runtime runtime;
        runtime.active_cells = true;
        runtime.words[Runtime::frame + 0x144u] = 0xc000u;
        runtime.words[Runtime::frame + 0x148u] = 0xc004u;
        AddActiveRecord(runtime, 0xc000u, 0xd000u, 11u,
                        2u, 0x7000u, 20);
        runtime.fail_at = fail;
        bool threw = false;
        try {
            (void)pal4::TradeWindowCharacterPages::UpdateActive(
                Runtime::frame, 2u, runtime);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        Check(threw && runtime.allocation_count == fail &&
                  runtime.cells.size() == fail - 1u &&
                  runtime.cleanup_calls == 1u,
              "active allocation failure unwinds before null insertion");
    }
}

struct Navigation final : pal4::ITradeWindowCharacterNavigationRuntime {
    Runtime& pages;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    bool active = false;
    explicit Navigation(Runtime& value) : pages(value) {}
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == Runtime::frame + 0xf8u, "navigation mode");
        return active ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return pages.ReadWord(address);
    }
    void FillActiveCharacter(std::uint32_t,
                             std::uint32_t) override {
        Check(false, "adapter must compose active page");
    }
    void FillInactiveCharacter(std::uint32_t,
                               std::uint32_t) override {
        Check(false, "adapter must compose inactive page");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        return pages.RowCount(list);
    }
    std::uint32_t NextCharacterIndex(std::uint32_t index) override {
        return (index + 1u) % 3u;
    }
    std::uint32_t PreviousCharacterIndex(std::uint32_t index) override {
        return (index + 2u) % 3u;
    }
    bool HasTradeItems(std::uint32_t) override { return true; }
    bool CanExecuteTrade(std::uint32_t) override { return true; }
    void SetVisible(std::uint32_t window, bool visible) override {
        visibility.emplace_back(window, visible);
    }
};

void CheckNavigationComposition() {
    Runtime pages;
    pages.only_page = 1u;
    pages.words[Runtime::frame + 0x44u] = 0x6000u;
    pages.words[Runtime::frame + 0x48u] = 0x6100u;
    AddElement(pages, 0xc000u, 0xd000u, 11u, 4, 0x7000u,
               20, true);
    Navigation navigation(pages);
    pal4::TradeWindowCharacterPageNavigationRuntime adapter(
        navigation, pages);
    pal4::TradeWindowCharacterNavigation::Update(
        Runtime::frame, 0u, true, adapter);
    Check(pages.words.at(Runtime::frame + 0xf0u) == 1u &&
              pages.rows == 1u && pages.cells.size() == 4u &&
              pages.bottom_image == "gongjiwupin" &&
              navigation.visibility ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0x6000u, true}, {0x6100u, true}}),
          "character navigation reaches populated inactive page");

    Runtime active_pages;
    active_pages.active_cells = true;
    active_pages.words[Runtime::frame + 0x44u] = 0x6000u;
    active_pages.words[Runtime::frame + 0x48u] = 0x6100u;
    active_pages.words[Runtime::frame + 0x144u] = 0xc000u;
    active_pages.words[Runtime::frame + 0x148u] = 0xc004u;
    AddActiveRecord(active_pages, 0xc000u, 0xd000u, 11u,
                    1u, 0x7000u, 20);
    Navigation active_navigation(active_pages);
    active_navigation.active = true;
    pal4::TradeWindowCharacterPageNavigationRuntime active_adapter(
        active_navigation, active_pages);
    pal4::TradeWindowCharacterNavigation::Update(
        Runtime::frame, 0u, true, active_adapter);
    Check(active_pages.words.at(Runtime::frame + 0xf0u) == 1u &&
              active_pages.rows == 1u &&
              active_pages.cells.size() == 4u &&
              active_navigation.visibility.size() == 2u,
          "character navigation reaches populated active page");
}

struct DisplayRuntime final : pal4::ITradeWindowCharacterItemDisplayRuntime {
    Runtime& pages;
    std::vector<std::pair<std::uint32_t, std::string>> shown;
    std::uint32_t model_frame = 0u;
    std::string model_path;
    unsigned cleanups = 0u;
    explicit DisplayRuntime(Runtime& value) : pages(value) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        return pages.ReadWord(address);
    }
    std::uint32_t InventoryBase() override {
        return pages.InventoryBase();
    }
    std::string ReadText(std::uint32_t) override { return ""; }
    std::string ConvertToUtf8(std::string_view text) override {
        return std::string(text);
    }
    void SetWindowText(std::uint32_t window,
                       std::string_view text) override {
        shown.emplace_back(window, text);
    }
    void SetGameObjectDisplay(std::uint32_t frame,
                              std::string_view, std::string_view,
                              std::string_view path) override {
        model_frame = frame;
        model_path = path;
    }
    std::uint32_t CleanupTextBuffer() override {
        ++cleanups;
        return 0x66u;
    }
};

void CheckEmptyPageDisplayComposition() {
    Runtime pages;
    pages.words[Runtime::frame + 0x2cu] = 0x6000u;
    pages.words[Runtime::frame + 0x30u] = 0x6100u;
    pages.words[Runtime::frame + 0x34u] = 0x6200u;
    DisplayRuntime display(pages);
    pal4::TradeWindowCharacterPageDisplayRuntime adapter(pages, display);
    Check(pal4::TradeWindowCharacterPages::UpdateActive(
              Runtime::frame, 2u, adapter) == 0x55u &&
              display.shown ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x6000u, ""}, {0x6200u, "100"},
                      {0x6100u, ""}}) &&
              display.model_frame == Runtime::frame &&
              display.model_path.empty() && display.cleanups == 1u &&
              pages.bottom_image == "fuzhuwuping" &&
              pages.cleanup_calls == 1u,
          "empty active page composes with character item display");
}

}  // namespace

int main() {
    try {
        CheckGateAndEmptyPages();
        CheckRowsAndSpinner();
        CheckAllocationFailure();
        CheckActivePages();
        CheckActiveAllocationFailure();
        CheckNavigationComposition();
        CheckEmptyPageDisplayComposition();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
