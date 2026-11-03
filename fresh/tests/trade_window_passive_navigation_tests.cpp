#include "pal4/trade_window_passive_navigation.h"

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

struct Runtime final : pal4::ITradeWindowPassiveNavigationRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {frame + 0x28u, 0x2000u}, {frame + 0x44u, 0x3000u},
        {frame + 0x48u, 0x4000u}};
    std::vector<std::uint32_t> rows{1u};
    std::size_t row_index = 0u;
    std::vector<std::uint32_t> pages;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::string> calls;
    bool active = true;
    bool has_items = true;
    bool can_execute = true;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == frame + 0xf8u, "active flag address");
        calls.emplace_back("active");
        return active ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void ShowActivePage(std::uint32_t owner,
                        std::uint32_t index) override {
        Check(owner == frame, "active page frame");
        calls.emplace_back("page:" + std::to_string(index));
        pages.push_back(index);
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x2000u, "list target");
        calls.emplace_back("rows");
        return rows.at(row_index++);
    }
    std::uint32_t NextIndex(std::uint32_t owner,
                            std::uint32_t index) override {
        Check(owner == frame, "next frame");
        calls.emplace_back("next");
        return (index + 1u) % 3u;
    }
    std::uint32_t PreviousIndex(std::uint32_t owner,
                                std::uint32_t index) override {
        Check(owner == frame, "previous frame");
        calls.emplace_back("previous");
        return (index + 2u) % 3u;
    }
    bool HasTradeItems(std::uint32_t owner) override {
        Check(owner == frame, "has-items frame");
        calls.emplace_back("has-items");
        return has_items;
    }
    bool CanExecuteTrade(std::uint32_t owner) override {
        Check(owner == frame, "can-execute frame");
        calls.emplace_back("can-execute");
        return can_execute;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        calls.emplace_back("visible:" + std::to_string(window));
        visibility.emplace_back(window, visible);
    }
};

void CheckImmediateAndInactive() {
    Runtime active;
    pal4::TradeWindowPassiveNavigation::Update(
        Runtime::frame, 2u, true, active);
    Check(active.pages == std::vector<std::uint32_t>{2u} &&
          active.visibility ==
              std::vector<std::pair<std::uint32_t, bool>>{
                  {0x3000u, true}, {0x4000u, true}} &&
          active.calls == std::vector<std::string>{
              "active", "page:2", "read:4136", "rows",
              "can-execute", "read:4164", "visible:12288",
              "read:4168", "visible:16384"},
          "immediate page and two arrows in call order");

    Runtime inactive;
    inactive.active = false;
    inactive.can_execute = false;
    pal4::TradeWindowPassiveNavigation::Update(
        Runtime::frame, 1u, false, inactive);
    Check(inactive.pages.empty() && inactive.row_index == 1u &&
          inactive.visibility ==
              std::vector<std::pair<std::uint32_t, bool>>{
                  {0x3000u, false}, {0x4000u, false}},
          "inactive path leaves current list and hides arrows");
}

void CheckSearchAndFallback() {
    Runtime forward;
    forward.rows = {0u, 0u, 1u};
    pal4::TradeWindowPassiveNavigation::Update(
        Runtime::frame, 0u, true, forward);
    Check(forward.pages == std::vector<std::uint32_t>{0u, 1u, 2u} &&
          forward.row_index == 3u &&
          forward.calls[4] == "next",
          "forward visits pages until rows appear");

    Runtime previous;
    previous.rows = {0u, 2u};
    pal4::TradeWindowPassiveNavigation::Update(
        Runtime::frame, 0u, false, previous);
    Check(previous.pages == std::vector<std::uint32_t>{0u, 2u} &&
          previous.row_index == 2u,
          "reverse search uses previous index");

    Runtime fallback;
    fallback.rows = {0u, 0u, 0u};
    fallback.has_items = false;
    pal4::TradeWindowPassiveNavigation::Update(
        Runtime::frame, 1u, true, fallback);
    Check(fallback.pages == std::vector<std::uint32_t>{1u, 2u, 0u, 0u} &&
          fallback.row_index == 3u &&
          fallback.visibility.size() == 2u,
          "full cycle with no trade items resets page zero");

    Runtime remaining;
    remaining.rows = {0u, 0u, 0u};
    pal4::TradeWindowPassiveNavigation::Update(
        Runtime::frame, 1u, true, remaining);
    Check(remaining.pages == std::vector<std::uint32_t>{1u, 2u, 0u},
          "remaining items skip fallback");

    Runtime inactive_fallback;
    inactive_fallback.active = false;
    inactive_fallback.rows = {0u, 0u, 0u};
    inactive_fallback.has_items = false;
    pal4::TradeWindowPassiveNavigation::Update(
        Runtime::frame, 1u, true, inactive_fallback);
    Check(inactive_fallback.pages == std::vector<std::uint32_t>{0u},
          "inactive full-cycle fallback still calls active page filler");
}

}  // namespace

int main() {
    try {
        CheckImmediateAndInactive();
        CheckSearchAndFallback();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
