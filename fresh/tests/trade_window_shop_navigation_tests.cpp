#include "pal4/trade_window_shop_navigation.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITradeWindowShopNavigationRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1028u, 0x2000u}, {0x1044u, 0x3000u},
        {0x1048u, 0x4000u}};
    std::uint8_t active = 0u;
    std::vector<std::uint32_t> row_counts;
    std::size_t row_read = 0u;
    std::vector<std::string> calls;
    bool has_items = true;
    bool can_execute = true;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x10f8u, "active flag offset");
        calls.emplace_back("read-active");
        return active;
    }
    void ShowActivePage(std::uint32_t frame,
                        std::uint32_t index) override {
        Check(frame == 0x1000u, "active page frame");
        calls.emplace_back("active:" + std::to_string(index));
    }
    void ShowInactivePage(std::uint32_t frame,
                          std::uint32_t index) override {
        Check(frame == 0x1000u, "inactive page frame");
        calls.emplace_back("inactive:" + std::to_string(index));
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x2000u, "list offset");
        calls.emplace_back("rows");
        return row_counts.empty() ? 0u :
            row_counts.at(row_read++);
    }
    std::uint32_t NextIndex(std::uint32_t frame,
                            std::uint32_t index) override {
        Check(frame == 0x1000u, "next frame");
        calls.emplace_back("next");
        return (index + 1u) % 3u;
    }
    std::uint32_t PrevIndex(std::uint32_t frame,
                            std::uint32_t index) override {
        Check(frame == 0x1000u, "previous frame");
        calls.emplace_back("prev");
        return (index + 2u) % 3u;
    }
    bool HasTradeItems(std::uint32_t frame) override {
        Check(frame == 0x1000u, "has-items frame");
        calls.emplace_back("has-items");
        return has_items;
    }
    bool CanExecuteTrade(std::uint32_t frame) override {
        Check(frame == 0x1000u, "can-execute frame");
        calls.emplace_back("can-execute");
        return can_execute;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        calls.emplace_back("visible:" + std::to_string(window) +
                           ":" + std::to_string(visible));
    }
};

void CheckFirstPage() {
    Runtime runtime;
    runtime.active = 1u;
    runtime.row_counts = {3u};
    pal4::TradeWindowShopNavigation::UpdateShopData(
        0x1000u, 2u, true, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read-word", "read-active", "active:2", "rows",
              "can-execute", "read-word", "visible:12288:1",
              "read-word", "visible:16384:1"},
          "populated active page leaves navigation unchanged");
}

void CheckEmptyPages() {
    Runtime runtime;
    runtime.active = 0u;
    runtime.can_execute = false;
    runtime.has_items = false;
    pal4::TradeWindowShopNavigation::UpdateShopData(
        0x1000u, 1u, false, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read-word", "read-active", "inactive:1", "rows", "prev",
              "read-active", "inactive:0", "rows", "prev",
              "read-active", "inactive:2", "rows", "prev", "has-items",
              "read-active", "inactive:0", "can-execute", "read-word",
              "visible:12288:0", "read-word", "visible:16384:0"},
          "empty cycle falls back to page zero and hides both arrows");
}

void CheckCycleWithItems() {
    Runtime runtime;
    runtime.active = 1u;
    pal4::TradeWindowShopNavigation::UpdateShopData(
        0x1000u, 0u, true, runtime);
    unsigned pages = 0u;
    for (const auto& call : runtime.calls)
        if (call.starts_with("active:")) ++pages;
    Check(pages == 3u &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "has-items") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "active:0") != runtime.calls.end(),
          "cycle with items stops without extra fallback page");
}

void CheckFindsLaterPage() {
    Runtime runtime;
    runtime.row_counts = {0u, 2u};
    pal4::TradeWindowShopNavigation::UpdateShopData(
        0x1000u, 0u, true, runtime);
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "inactive:1") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "inactive:2") == runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "has-items") == runtime.calls.end(),
          "first nonempty page ends navigation");
}

}  // namespace

int main() {
    try {
        CheckFirstPage();
        CheckEmptyPages();
        CheckCycleWithItems();
        CheckFindsLaterPage();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
