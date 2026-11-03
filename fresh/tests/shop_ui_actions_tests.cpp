#include "pal4/shop_ui_actions.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShopUiActionsRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5000u, 0x6000u}, {0x6004u, 0x7000u}};
    std::vector<std::string> calls;
    std::uint32_t category = 0xffffffffu;
    std::uint32_t buy = 0xffffffffu;

    void GetData(const std::uint32_t frame,
                 const std::uint32_t selected_category,
                 const std::uint32_t selected_buy) override {
        Check(frame == 0x5000u, "data selection frame receiver");
        calls.emplace_back("data");
        category = selected_category;
        buy = selected_buy;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void CallVisibilitySlot(const std::uint32_t function,
                            const std::uint32_t frame,
                            const std::uint32_t visible,
                            const std::uint32_t argument) override {
        Check(function == 0x7000u && frame == 0x5000u &&
                  visible == 0u && argument == 0u,
              "close invokes frame vtable +4 with two zero arguments");
        calls.emplace_back("hide");
    }
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade system");
        return 0x8000u;
    }
    std::uint32_t SetTradeState(
        const std::uint32_t trade_system) override {
        Check(trade_system == 0x8000u,
              "close forwards singleton to trade-state setter");
        calls.emplace_back("trade state");
        return 0xabcd1234u;
    }
};

using Click = std::uint8_t (*)(std::uint32_t, std::uint32_t,
                               pal4::IShopUiActionsRuntime&);

void TestSelectionCallbacks() {
    constexpr std::array<Click, 5> callbacks{
        &pal4::ShopUiActions::OnBuyEquip,
        &pal4::ShopUiActions::OnSellEquip,
        &pal4::ShopUiActions::OnBuyProperty,
        &pal4::ShopUiActions::OnSellProperty,
        &pal4::ShopUiActions::OnBuyDescription};
    constexpr std::array<std::uint32_t, 5> categories{0u, 0u, 1u, 1u, 2u};
    constexpr std::array<std::uint32_t, 5> buys{1u, 0u, 1u, 0u, 1u};
    for (std::size_t i = 0u; i < callbacks.size(); ++i) {
        Runtime runtime;
        Check(callbacks[i](0x5000u, 0xdeadbeefu, runtime) == 1u &&
                  runtime.category == categories[i] &&
                  runtime.buy == buys[i] &&
                  runtime.calls == std::vector<std::string>{"data"},
              "shop selection callback passes exact mode and ignores event");
    }
}

void TestCloseChain() {
    Runtime runtime;
    Check(pal4::ShopUiActions::Close(0x5000u, runtime) ==
              0xabcd1234u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "read", "hide", "trade system", "trade state"}),
          "close hides by virtual slot then returns trade-state result");

    runtime.calls.clear();
    Check(pal4::ShopUiActions::OnClose(
              0x5000u, 0xdeadbeefu, runtime) == 1u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "read", "hide", "trade system", "trade state"}),
          "close click composes close and returns one");
}

}  // namespace

int main() {
    TestSelectionCallbacks();
    TestCloseChain();
    return EXIT_SUCCESS;
}
