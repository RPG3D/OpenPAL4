#include "pal4/inventory_trade.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string_view operation;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    std::uint32_t third = 0u;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IInventoryTradeRuntime {
    std::vector<Call> calls;
    std::uint32_t buy_record = 0x2000u;
    std::uint32_t sell_record = 0x4000u;
    std::uint32_t price = 20u;
    std::uint32_t balance = 30u;
    std::uint32_t proceeds = 5u;
    std::int32_t removal_result = 0x1234;

    std::uint32_t GameDatabaseInstance() override {
        calls.push_back({"database"});
        return 0x5000u;
    }
    std::uint32_t FindBuyRecord(const std::uint32_t table,
                                const std::uint32_t item_id) override {
        calls.push_back({"buy-record", table, item_id});
        return buy_record;
    }
    std::uint32_t Inventory() override {
        calls.push_back({"inventory"});
        return 0x3000u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back({"read", address});
        if (address == 0x3134u) return balance;
        Check(address == 0x2178u || address == 0x4178u,
              "trade price uses record offset 376");
        return price;
    }
    void UpdateCounter(const std::uint32_t inventory,
                       const std::uint32_t delta) override {
        calls.push_back({"counter", inventory, delta});
    }
    void AddItem(const std::uint32_t collection,
                 const std::uint32_t item_id) override {
        calls.push_back({"add", collection, item_id});
    }
    std::uint32_t FindSellRecord(
        const std::uint32_t collection, const std::uint32_t item_id,
        const std::uint32_t category) override {
        calls.push_back({"sell-record", collection, item_id, category});
        return sell_record;
    }
    std::uint32_t QuarterPrice(const std::uint32_t signed_price_bits) override {
        calls.push_back({"quarter-price", signed_price_bits});
        return proceeds;
    }
    std::int32_t RemoveItem(const std::uint32_t collection,
                            const std::uint32_t record) override {
        calls.push_back({"remove", collection, record});
        return removal_result;
    }
};

void BuyPaths() {
    Runtime success;
    Check(pal4::InventoryTrade::Buy(7u, success) == 0 &&
              success.calls == std::vector<Call>{
                  {"database"}, {"buy-record", 0x5058u, 7u},
                  {"inventory"}, {"read", 0x2178u},
                  {"read", 0x3134u}, {"inventory"},
                  {"counter", 0x3000u, 0xffffffecu},
                  {"inventory"}, {"add", 0x307cu, 7u}},
          "buy deducts price then adds the item with exact offsets");

    Runtime missing;
    missing.buy_record = 0u;
    Check(pal4::InventoryTrade::Buy(7u, missing) == 0 &&
              missing.calls == std::vector<Call>{
                  {"database"}, {"buy-record", 0x5058u, 7u}},
          "missing buy record returns without touching inventory");

    Runtime short_balance;
    short_balance.balance = 19u;
    Check(pal4::InventoryTrade::Buy(7u, short_balance) == 45 &&
              short_balance.calls.size() == 5u &&
              short_balance.calls.back() == Call{"read", 0x3134u},
          "insufficient funds return 45 before mutation");

    Runtime signed_price;
    signed_price.balance = 0u;
    signed_price.price = 0xffffffffu;
    Check(pal4::InventoryTrade::Buy(7u, signed_price) == 0 &&
              signed_price.calls[6] == Call{"counter", 0x3000u, 1u},
          "balance comparison and negation preserve signed x86 behavior");
}

void SellPaths() {
    Runtime success;
    Check(pal4::InventoryTrade::Sell(7u, success) == 0x34u &&
              success.calls == std::vector<Call>{
                  {"inventory"}, {"sell-record", 0x307cu, 7u, 7u},
                  {"read", 0x4178u}, {"quarter-price", 20u},
                  {"inventory"}, {"counter", 0x3000u, 5u},
                  {"inventory"}, {"remove", 0x307cu, 0x4000u}},
          "sell credits quarter price before removing item");

    Runtime missing;
    missing.sell_record = 0u;
    Check(pal4::InventoryTrade::Sell(7u, missing) == 0u &&
              missing.calls == std::vector<Call>{
                  {"inventory"}, {"sell-record", 0x307cu, 7u, 7u}},
          "missing sell record leaves balance and inventory unchanged");
}

}  // namespace

int main() {
    try {
        BuyPaths();
        SellPaths();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
