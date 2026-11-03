#include "pal4/shop_ui_data.h"
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

struct Runtime final : pal4::IShopUiDataRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5000u, 0x6000u}, {0x6004u, 0x7000u}};
    std::vector<std::string> calls;
    std::uint32_t trade_reads = 0u;
    std::uint32_t expected_category = 0u;
    std::uint8_t expected_buy = 0u;

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
              "data loading hides shop by virtual slot +4");
        calls.emplace_back("hide");
    }
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade system");
        return trade_reads++ == 0u ? 0x8000u : 0x8100u;
    }
    void EnsureGameDatabase(const std::uint32_t trade_system) override {
        Check(trade_system == 0x8000u,
              "GameDB getter uses first trade-system instance result");
        calls.emplace_back("database");
    }
    std::uint8_t OpenShop(
        const std::uint32_t trade_system, const std::uint32_t data,
        const std::uint8_t buy,
        const std::uint32_t category) override {
        Check(trade_system == 0x8100u && data == 0x5060u &&
                  buy == expected_buy && category == expected_category,
              "effect/music handler receives second trade system and +0x60");
        calls.emplace_back("play");
        return 0x7au;
    }
};

void TestRawArgumentsAndOrder() {
    for (const auto buy : {0u, 1u}) {
        Runtime runtime;
        runtime.expected_category = 0x87654321u;
        runtime.expected_buy = static_cast<std::uint8_t>(buy);
        Check(pal4::ShopUiData::Load(
                  0x5000u, runtime.expected_category,
                  runtime.expected_buy, runtime) == 0x7au &&
                  runtime.trade_reads == 2u &&
                  runtime.calls == (std::vector<std::string>{
                      "read", "read", "hide", "trade system",
                      "database", "trade system", "play"}),
              "shop data loader preserves category, byte mode and call order");
    }
}

void TestSelectionComposition() {
    using Click = std::uint8_t (*)(
        std::uint32_t, std::uint32_t, pal4::IShopUiDataRuntime&);
    const std::array<Click, 5> callbacks{
        static_cast<Click>(&pal4::ShopUiActions::OnBuyEquip),
        static_cast<Click>(&pal4::ShopUiActions::OnSellEquip),
        static_cast<Click>(&pal4::ShopUiActions::OnBuyProperty),
        static_cast<Click>(&pal4::ShopUiActions::OnSellProperty),
        static_cast<Click>(&pal4::ShopUiActions::OnBuyDescription)};
    constexpr std::array<std::uint32_t, 5> categories{
        0u, 0u, 1u, 1u, 2u};
    constexpr std::array<std::uint8_t, 5> modes{
        1u, 0u, 1u, 0u, 1u};
    for (std::size_t i = 0u; i < callbacks.size(); ++i) {
        Runtime runtime;
        runtime.expected_category = categories[i];
        runtime.expected_buy = modes[i];
        Check(callbacks[i](0x5000u, 0xdeadbeefu, runtime) == 1u &&
                  runtime.calls.back() == "play" &&
                  runtime.trade_reads == 2u,
              "five click callbacks compose reconstructed data loader");
    }
}

}  // namespace

int main() {
    TestRawArgumentsAndOrder();
    TestSelectionComposition();
    return EXIT_SUCCESS;
}
