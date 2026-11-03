#include "pal4/trade_window_passive_item_display.h"
#include "pal4/trade_window_passive_page_display_runtime.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITradeWindowPassiveItemDisplayRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t item = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {frame + 0x2cu, 0x3000u}, {frame + 0x30u, 0x3100u},
        {frame + 0x34u, 0x3200u}, {frame + 0x38u, 0x3300u},
        {item + 0x18u, 0x4000u}, {item + 0x28u, 0x4100u},
        {item + 0x64u, 0u}, {item + 0x68u, 0x42u},
        {item + 0x78u, static_cast<std::uint32_t>(-7)},
        {0x9134u, static_cast<std::uint32_t>(-50)}};
    std::map<std::uint32_t, std::string> texts{
        {0x4000u, "Name"}, {0x4100u, "Direct description"}};
    std::vector<std::pair<std::uint32_t, std::string>> shown;
    std::vector<std::string> calls;
    std::uint32_t db_key = 0u;
    std::uint32_t db_item = 0u;
    std::uint32_t model_frame = 0u;
    std::string model_name;
    std::string texture;
    std::string path;
    bool throw_on_description_text = false;
    unsigned description_cleanups = 0u;
    unsigned buffer_cleanups = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t InventoryBase() override {
        calls.emplace_back("inventory-base");
        return 0x9000u;
    }
    std::string ReadText(std::uint32_t address) override {
        calls.emplace_back("read-text");
        return address == 0u ? "" : texts.at(address);
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("utf8");
        return std::string(text);
    }
    std::uint32_t FindGameDbItem(std::uint32_t key) override {
        db_key = key;
        calls.emplace_back("db");
        return 0xa000u;
    }
    std::string DescribeGameDbItem(std::uint32_t value) override {
        db_item = value;
        calls.emplace_back("describe");
        return "Generated description";
    }
    void CleanupDescription() override {
        ++description_cleanups;
        calls.emplace_back("description-cleanup");
    }
    void SetWindowText(std::uint32_t window,
                       std::string_view text) override {
        if (throw_on_description_text && window == 0x3100u)
            throw std::runtime_error("set description failed");
        shown.emplace_back(window, std::string(text));
        calls.emplace_back("set-text");
    }
    void SetGameObjectDisplay(std::uint32_t owner,
                              std::string_view name,
                              std::string_view value,
                              std::string_view object_path) override {
        model_frame = owner;
        model_name = name;
        texture = value;
        path = object_path;
        calls.emplace_back("model");
    }
    std::uint32_t CleanupTextBuffer() override {
        ++buffer_cleanups;
        calls.emplace_back("buffer-cleanup");
        return 0x55u;
    }
};

void CheckEmptyItem() {
    Runtime runtime;
    const auto result = pal4::TradeWindowPassiveItemDisplay::Update(
        Runtime::frame, 0u, runtime);
    Check(result == 0x55u && runtime.shown ==
              (std::vector<std::pair<std::uint32_t, std::string>>{
                  {0x3000u, ""}, {0x3100u, ""},
                  {0x3200u, "-50"}, {0x3300u, ""}}),
          "empty item clears three labels and preserves signed balance");
    Check(runtime.db_key == 0u && runtime.model_name.empty() &&
              runtime.description_cleanups == 0u &&
              runtime.buffer_cleanups == 1u &&
              runtime.calls.back() == "buffer-cleanup",
          "empty item avoids lookup and model but cleans text buffer");
}

void CheckCategories() {
    constexpr const char* textures[] = {"ZZA1", "ZZA2", "ZZA3"};
    for (std::uint32_t category = 0u; category <= 3u; ++category) {
        Runtime runtime;
        runtime.words[Runtime::item + 0x64u] = category;
        const auto result = pal4::TradeWindowPassiveItemDisplay::Update(
            Runtime::frame, Runtime::item, runtime);
        Check(result == 0x55u && runtime.shown ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x3000u, "Name"},
                      {0x3100u, category == 0u ? "Generated description"
                                                   : "Direct description"},
                      {0x3200u, "-50"}, {0x3300u, "-7"}}),
              "four text controls and signed values");
        Check((category == 0u && runtime.db_key == 0x42u &&
               runtime.db_item == 0xa000u &&
               runtime.description_cleanups == 1u) ||
                  (category != 0u && runtime.db_key == 0u &&
                   runtime.description_cleanups == 0u),
              "only category zero generates GameDB description");
        if (category < 3u) {
            Check(runtime.model_frame == Runtime::frame &&
                      runtime.model_name == "zza" &&
                      runtime.texture == textures[category] &&
                      runtime.path == "PALObject\\",
                  "category-specific preview model");
        } else {
            Check(runtime.model_frame == 0u,
                  "unsupported category leaves preview untouched");
        }
        Check(runtime.calls.back() == "buffer-cleanup",
              "last action cleans temporary text buffer");
    }
}

void CheckMissingTextAndExceptionCleanup() {
    Runtime missing;
    missing.words[Runtime::item + 0x18u] = 0u;
    missing.words[Runtime::item + 0x64u] = 1u;
    missing.words[Runtime::item + 0x28u] = 0u;
    (void)pal4::TradeWindowPassiveItemDisplay::Update(
        Runtime::frame, Runtime::item, missing);
    Check(missing.shown[0].second.empty() &&
              missing.shown[1].second.empty(),
          "null native strings convert as empty text");

    Runtime failure;
    failure.throw_on_description_text = true;
    bool threw = false;
    try {
        (void)pal4::TradeWindowPassiveItemDisplay::Update(
            Runtime::frame, Runtime::item, failure);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failure.description_cleanups == 1u &&
              failure.buffer_cleanups == 0u &&
              failure.calls.back() == "description-cleanup",
          "description temporary cleans up when text binding throws");
}

struct EmptyPageRuntime final : pal4::ITradeWindowPassivePageRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {Runtime::frame + 0x28u, 0x5000u},
        {Runtime::frame + 0xe4u, 0x5100u},
        {Runtime::frame + 0x190u, 0x5200u},
        {Runtime::frame + 0x154u, 0u},
        {Runtime::frame + 0x158u, 0u},
        {Runtime::frame + 0x50u, 0x5300u}};
    std::uint32_t columns = 0u;
    std::string bottom_image;
    bool reset = false;
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == Runtime::frame + 0xf8u, "page active flag");
        return 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {
        Check(false, "empty page has no cells");
    }
    bool IsTradeList(std::uint32_t list) override {
        Check(list == 0x5000u, "page list");
        return true;
    }
    void SetImage(std::uint32_t window, std::uint32_t image) override {
        Check(window == 0x5100u && image == 0x5200u,
              "page tab image");
    }
    void SetNamedImage(std::uint32_t window,
                       std::string_view imageset,
                       std::string_view image) override {
        Check(window == 0x5300u && imageset == "zi0",
              "page bottom image");
        bottom_image = image;
    }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x5000u, "reset empty page list");
        reset = true;
    }
    std::uint32_t ColumnCount(std::uint32_t) override { return 0u; }
    void RemoveColumn(std::uint32_t, std::uint32_t) override {
        Check(false, "empty page has no old columns");
    }
    void AddColumn(std::uint32_t, std::uint32_t,
                   float) override { ++columns; }
    void SetColumnLabelImage(std::uint32_t, std::uint32_t,
                             std::string_view,
                             std::string_view) override {}
    std::uint32_t FindGameDbItem(std::uint32_t) override {
        Check(false, "empty page does not query database");
        return 0u;
    }
    std::uint32_t FindEffect(std::uint32_t) override {
        Check(false, "empty page does not query effect");
        return 0u;
    }
    std::uint32_t InventorySearch(std::uint32_t) override {
        Check(false, "empty page does not query inventory");
        return 0u;
    }
    std::uint32_t InventoryBase() override { return 0u; }
    std::string Utf8Name(std::uint32_t) override { return {}; }
    void AddRow(std::uint32_t) override {
        Check(false, "empty page adds no row");
    }
    std::uint32_t RowCount(std::uint32_t) override { return 0u; }
    std::uint32_t CreateItem(std::uint32_t, std::uint32_t,
                             std::string_view) override {
        Check(false, "empty page creates no cell");
        return 0u;
    }
    void SetItem(std::uint32_t, std::uint32_t, std::uint32_t,
                 std::uint32_t) override {
        Check(false, "empty page inserts no cell");
    }
    void SetHoverRowIndex(std::uint32_t, std::int32_t) override {
        Check(false, "empty page has no hover row");
    }
    void ClearItemDisplay(std::uint32_t) override {
        Check(false, "adapter must compose item display");
    }
};

void CheckEmptyPageComposition() {
    Runtime display;
    EmptyPageRuntime page;
    pal4::TradeWindowPassivePageDisplayRuntime adapter(page, display);
    pal4::TradeWindowPassivePages::Update(Runtime::frame, 0u, adapter);
    Check(page.reset && page.columns == 4u &&
              page.bottom_image == "rongzhutupu" &&
              page.words.at(Runtime::frame + 0xf4u) == 0u &&
              display.shown ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x3000u, ""}, {0x3100u, ""},
                      {0x3200u, "-50"}, {0x3300u, ""}}) &&
              display.buffer_cleanups == 1u,
          "empty passive page composes real display reset");
}

}  // namespace

int main() {
    try {
        CheckEmptyItem();
        CheckCategories();
        CheckMissingTextAndExceptionCleanup();
        CheckEmptyPageComposition();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
