#include "pal4/trade_window_hover_lookup.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITradeWindowHoverLookupRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {frame + 0xe8u, 0u}, {frame + 0x28u, 0x2000u},
        {0x3000u + 0x138u, 42u}};
    std::vector<std::string> calls;
    std::uint32_t hovered = 0x3000u;
    std::uint32_t db_result = 0x4000u;
    std::uint32_t passive_result = 0x4500u;
    std::uint32_t character_result = 0x4800u;
    std::uint32_t inventory_result = 0x5000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t GetHoverItem(std::uint32_t list) override {
        Check(list == 0x2000u, "hover list");
        calls.emplace_back("hover");
        return hovered;
    }
    std::uint32_t GameDbBase() override {
        calls.emplace_back("db");
        return 0x6000u;
    }
    std::uint32_t InventoryBase() override {
        calls.emplace_back("inventory");
        return 0x7000u;
    }
    std::uint32_t FindGameDbItem(std::uint32_t collection,
                                 std::uint32_t id) override {
        Check(collection == 0x6058u && id == words.at(0x3138u),
              "game database collection and id");
        calls.emplace_back("find-db");
        return db_result;
    }
    std::uint32_t FindGameDbPassiveItem(
        std::uint32_t collection, std::uint32_t id) override {
        Check(collection == 0x6080u && id == words.at(0x3138u),
              "passive database collection and id");
        calls.emplace_back("find-passive");
        return passive_result;
    }
    std::uint32_t FindInventoryItem(std::uint32_t collection,
                                    std::uint32_t id,
                                    std::uint32_t category) override {
        Check(id == words.at(0x3138u) &&
                  ((collection == 0x707cu && category == 7u) ||
                   (collection == 0x7000u && category == 5u)),
              "inventory collection, id and category");
        calls.emplace_back("find-inventory");
        return inventory_result;
    }
    std::uint32_t FindGameDbCharacterItem(
        std::uint32_t database, std::uint32_t id) override {
        Check(database == 0x6000u && id == words.at(0x3138u),
              "character GameDB base and id");
        calls.emplace_back("find-character");
        return character_result;
    }
};

void CheckLookupPaths() {
    Runtime runtime;
    Check(pal4::TradeWindowHoverLookup::GetGameDbItem(
              Runtime::frame, runtime) == 0x4000u,
          "game database result");
    Check(runtime.calls == std::vector<std::string>{
              "read:4328", "read:4136", "hover", "read:12600",
              "db", "find-db"}, "game database call order");
    runtime.calls.clear();
    Check(pal4::TradeWindowHoverLookup::GetInventoryItem(
              Runtime::frame, runtime) == 0x5000u,
          "inventory result");
    Check(runtime.calls == std::vector<std::string>{
              "read:4328", "read:4136", "hover", "read:12600",
              "inventory", "find-inventory"}, "inventory call order");

    runtime.words[0x3138u] = 0u;
    Check(pal4::TradeWindowHoverLookup::GetGameDbItem(
              Runtime::frame, runtime) == 0x4000u,
          "zero id is still searched");
    runtime.db_result = 0u;
    Check(pal4::TradeWindowHoverLookup::GetGameDbItem(
              Runtime::frame, runtime) == 0u,
          "missing database record forwarded");
}

void CheckEarlyReturns() {
    Runtime runtime;
    runtime.words[Runtime::frame + 0xe8u] = 1u;
    Check(pal4::TradeWindowHoverLookup::GetGameDbItem(
              Runtime::frame, runtime) == 0u &&
          pal4::TradeWindowHoverLookup::GetInventoryItem(
              Runtime::frame, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{
              "read:4328", "read:4328"}, "blocked hover returns early");
    runtime.words[Runtime::frame + 0xe8u] = 0u;
    runtime.hovered = 0u;
    runtime.calls.clear();
    Check(pal4::TradeWindowHoverLookup::GetGameDbItem(
              Runtime::frame, runtime) == 0u &&
          pal4::TradeWindowHoverLookup::GetInventoryItem(
              Runtime::frame, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{
              "read:4328", "read:4136", "hover",
              "read:4328", "read:4136", "hover"},
          "absent hover skips both subsystems");
}

void CheckPassiveLookup() {
    Runtime runtime;
    Check(pal4::TradeWindowHoverLookup::GetPassiveItem(
              Runtime::frame, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"read:4328"},
          "passive lookup requires state two");
    runtime.words[Runtime::frame + 0xe8u] = 2u;
    runtime.calls.clear();
    Check(pal4::TradeWindowHoverLookup::GetPassiveItem(
              Runtime::frame, runtime) == 0x4500u &&
              runtime.calls == std::vector<std::string>{
                  "read:4328", "read:4136", "hover", "read:12600",
                  "db", "find-passive"},
          "passive lookup uses hovered ID and GameDB +0x80");
    runtime.words[0x3138u] = 0u;
    runtime.passive_result = 0u;
    Check(pal4::TradeWindowHoverLookup::GetPassiveItem(
              Runtime::frame, runtime) == 0u,
          "zero ID is searched and missing result forwarded");
    runtime.hovered = 0u;
    runtime.calls.clear();
    Check(pal4::TradeWindowHoverLookup::GetPassiveItem(
              Runtime::frame, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4328", "read:4136", "hover"},
          "absent passive hover skips database");
}

void CheckBuyLookups() {
    Runtime runtime;
    Check(pal4::TradeWindowHoverLookup::GetBuyItem(
              Runtime::frame, runtime) == 0u &&
              pal4::TradeWindowHoverLookup::GetBuyInventoryItem(
                  Runtime::frame, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4328", "read:4328"},
          "buy lookups require mode one");
    runtime.words[Runtime::frame + 0xe8u] = 1u;
    runtime.calls.clear();
    Check(pal4::TradeWindowHoverLookup::GetBuyItem(
              Runtime::frame, runtime) == 0x4800u &&
              runtime.calls == std::vector<std::string>{
                  "read:4328", "read:4136", "hover", "read:12600",
                  "db", "find-character"},
          "buy GameDB lookup uses database base and hovered ID");
    runtime.calls.clear();
    Check(pal4::TradeWindowHoverLookup::GetBuyInventoryItem(
              Runtime::frame, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "read:4328", "read:4136", "hover", "read:12600",
                  "inventory", "find-inventory"},
          "buy inventory lookup uses category five");
    runtime.hovered = 0u;
    runtime.calls.clear();
    Check(pal4::TradeWindowHoverLookup::GetBuyItem(
              Runtime::frame, runtime) == 0u &&
              pal4::TradeWindowHoverLookup::GetBuyInventoryItem(
                  Runtime::frame, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4328", "read:4136", "hover",
                  "read:4328", "read:4136", "hover"},
          "absent buy hover skips both data sources");
}

void CheckPageIndices() {
    for (std::int32_t index = 0; index < 7; ++index) {
        Check(pal4::TradeWindowPageIndex::Next(index) ==
                  (index + 1) % 7, "next page wrap");
        Check(pal4::TradeWindowPageIndex::Previous(index) ==
                  (index + 6) % 7, "previous page wrap");
    }
    Check(pal4::TradeWindowPageIndex::Next(-2) == -1 &&
          pal4::TradeWindowPageIndex::Next(9) == 0 &&
          pal4::TradeWindowPageIndex::Previous(-2) == 6 &&
          pal4::TradeWindowPageIndex::Previous(9) == 8,
          "out of range follows signed comparisons");
    Check(pal4::TradeWindowPageIndex::Next(
              std::numeric_limits<std::int32_t>::max()) ==
              std::numeric_limits<std::int32_t>::min() &&
          pal4::TradeWindowPageIndex::Previous(
              std::numeric_limits<std::int32_t>::min()) ==
              std::numeric_limits<std::int32_t>::max(),
          "x86 arithmetic wraps without undefined behavior");
}

}  // namespace

int main() {
    try {
        CheckLookupPaths();
        CheckEarlyReturns();
        CheckPassiveLookup();
        CheckBuyLookups();
        CheckPageIndices();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
