#include "pal4/trade_window_character_navigation.h"

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

struct Runtime final : pal4::ITradeWindowCharacterNavigationRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {frame + 0x28u, 0x2000u}, {frame + 0x44u, 0x3000u},
        {frame + 0x48u, 0x4000u}};
    std::vector<std::uint32_t> rows{1u};
    std::size_t row_index = 0u;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::string> calls;
    bool active = true;
    bool has_items = true;
    bool can_execute = true;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == frame + 0xf8u, "character mode offset");
        calls.emplace_back("mode");
        return active ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void FillActiveCharacter(std::uint32_t owner,
                             std::uint32_t index) override {
        Check(owner == frame, "active character frame");
        calls.emplace_back("active:" + std::to_string(index));
    }
    void FillInactiveCharacter(std::uint32_t owner,
                               std::uint32_t index) override {
        Check(owner == frame, "inactive character frame");
        calls.emplace_back("inactive:" + std::to_string(index));
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x2000u, "character list");
        calls.emplace_back("rows");
        return rows.at(row_index++);
    }
    std::uint32_t NextCharacterIndex(std::uint32_t index) override {
        calls.emplace_back("next");
        return (index + 1u) % 3u;
    }
    std::uint32_t PreviousCharacterIndex(std::uint32_t index) override {
        calls.emplace_back("previous");
        return (index + 2u) % 3u;
    }
    bool HasTradeItems(std::uint32_t owner) override {
        Check(owner == frame, "remaining trade frame");
        calls.emplace_back("has-items");
        return has_items;
    }
    bool CanExecuteTrade(std::uint32_t owner) override {
        Check(owner == frame, "can trade frame");
        calls.emplace_back("can-trade");
        return can_execute;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        calls.emplace_back("visible:" + std::to_string(window));
        visibility.emplace_back(window, visible);
    }
};

void CheckImmediateAndInactive() {
    Runtime active;
    pal4::TradeWindowCharacterNavigation::Update(
        Runtime::frame, 2u, true, active);
    Check(active.calls == std::vector<std::string>{
              "mode", "active:2", "read:4136", "rows", "can-trade",
              "read:4164", "visible:12288", "read:4168",
              "visible:16384"} &&
              active.visibility ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0x3000u, true}, {0x4000u, true}}),
          "active immediate page and arrow call order");

    Runtime inactive;
    inactive.active = false;
    inactive.can_execute = false;
    pal4::TradeWindowCharacterNavigation::Update(
        Runtime::frame, 1u, false, inactive);
    Check(inactive.calls[1] == "inactive:1" &&
              inactive.row_index == 1u &&
              inactive.visibility ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0x3000u, false}, {0x4000u, false}}),
          "inactive mode still fills and hides arrows when trading blocked");
}

void CheckTraversalAndFallback() {
    Runtime forward;
    forward.rows = {0u, 0u, 1u};
    pal4::TradeWindowCharacterNavigation::Update(
        Runtime::frame, 0u, true, forward);
    Check(forward.calls == std::vector<std::string>{
              "mode", "active:0", "read:4136", "rows", "next",
              "mode", "active:1", "read:4136", "rows", "next",
              "mode", "active:2", "read:4136", "rows", "can-trade",
              "read:4164", "visible:12288", "read:4168",
              "visible:16384"},
          "forward traversal stops at first populated page");

    Runtime reverse;
    reverse.rows = {0u, 1u};
    reverse.active = false;
    pal4::TradeWindowCharacterNavigation::Update(
        Runtime::frame, 0u, false, reverse);
    Check(reverse.calls[1] == "inactive:0" &&
              reverse.calls[4] == "previous" &&
              reverse.calls[6] == "inactive:2",
          "reverse traversal uses previous index in inactive mode");

    Runtime fallback;
    fallback.rows = {0u, 0u, 0u};
    fallback.has_items = false;
    pal4::TradeWindowCharacterNavigation::Update(
        Runtime::frame, 1u, true, fallback);
    std::vector<std::string> fallback_fills;
    for (const auto& call : fallback.calls)
        if (call.starts_with("active:")) fallback_fills.push_back(call);
    Check(fallback.row_index == 3u &&
              fallback_fills ==
                  (std::vector<std::string>{
                      "active:1", "active:2", "active:0", "active:0"}),
          "full cycle with no items fills page zero once more");

    Runtime remaining;
    remaining.rows = {0u, 0u, 0u};
    pal4::TradeWindowCharacterNavigation::Update(
        Runtime::frame, 1u, true, remaining);
    std::vector<std::string> remaining_fills;
    for (const auto& call : remaining.calls)
        if (call.starts_with("active:")) remaining_fills.push_back(call);
    Check(remaining.row_index == 3u &&
              remaining_fills ==
                  (std::vector<std::string>{
                      "active:1", "active:2", "active:0"}),
          "remaining items skip fallback fill");
}

}  // namespace

int main() {
    try {
        CheckImmediateAndInactive();
        CheckTraversalAndFallback();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
