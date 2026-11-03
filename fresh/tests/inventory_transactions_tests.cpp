#include "pal4/inventory_transactions.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
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

struct Runtime final : pal4::IInventoryTransactionsRuntime {
    std::vector<Call> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> item_records{
        {1u, 0x6000u}, {2u, 0x6100u}};
    std::map<std::uint32_t, std::uint32_t> inventory_entries{
        {1u, 0x9000u}, {2u, 0x9100u}};
    std::uint32_t recipe_record = 0x7000u;
    bool recipe_known = false;

    Runtime() {
        words[0x6090u] = 7u;
        words[0x6190u] = 11u;
        words[0x8134u] = 30u;
        words[0x9000u] = 0xa000u;
        words[0x9008u] = 4u;
        words[0xa000u] = 1u;
        words[0xa090u] = 9u;
        words[0x9100u] = 0xa100u;
        words[0x9108u] = 3u;
        words[0xa100u] = 2u;
        words[0xa190u] = 12u;
        words[0x7078u] = 25u;
        words[0x7064u] = 0xbeefu;
    }

    std::uint32_t GameDatabaseInstance() override {
        calls.push_back({"database"});
        return 0x5000u;
    }
    std::uint32_t FindItemRecord(const std::uint32_t database,
                                 const std::uint32_t item_id) override {
        Check(database == 0x5000u, "item search uses GameDB base");
        calls.push_back({"item-record", database, item_id});
        const auto found = item_records.find(item_id);
        return found == item_records.end() ? 0u : found->second;
    }
    std::uint32_t FindRecipeRecord(const std::uint32_t table,
                                   const std::uint32_t token) override {
        Check(table == 0x5080u, "recipe table is at GameDB offset 128");
        calls.push_back({"recipe-record", table, token});
        return recipe_record;
    }
    std::uint32_t Inventory() override {
        calls.push_back({"inventory"});
        return 0x8000u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back({"read", address});
        return words.at(address);
    }
    void UpdateCounter(const std::uint32_t inventory,
                       const std::uint32_t delta) override {
        Check(inventory == 0x8000u, "counter update inventory address");
        calls.push_back({"counter", inventory, delta});
        words[0x8134u] += delta;
    }
    void ProcessItem(const std::uint32_t inventory,
                     const std::uint32_t item_id,
                     const std::uint32_t quantity) override {
        calls.push_back({"process", inventory, item_id, quantity});
    }
    std::uint32_t FindInventoryEntry(
        const std::uint32_t inventory, const std::uint32_t item_id,
        const std::uint32_t category) override {
        Check(inventory == 0x8000u && category == 5u,
              "inventory entry lookup uses category five");
        calls.push_back({"entry", inventory, item_id, category});
        const auto found = inventory_entries.find(item_id);
        return found == inventory_entries.end() ? 0u : found->second;
    }
    void RemoveInventoryItem(const std::uint32_t inventory,
                             const std::uint32_t item_id,
                             const std::uint32_t quantity) override {
        calls.push_back({"remove", inventory, item_id, quantity});
        words.at(inventory_entries.at(item_id) + 8u) -= quantity;
    }
    bool HasRecipe(const std::uint32_t collection,
                   const std::uint32_t token) override {
        Check(collection == 0x8104u,
              "known recipes reside at inventory offset 260");
        calls.push_back({"has-recipe", collection, token});
        return recipe_known;
    }
    void LearnRecipe(const std::uint32_t collection,
                     const std::uint32_t learned_id,
                     const std::uint32_t token) override {
        calls.push_back({"learn", collection, learned_id, token});
        recipe_known = true;
    }
};

std::size_t Count(const Runtime& runtime, const std::string_view operation) {
    return static_cast<std::size_t>(std::count_if(
        runtime.calls.begin(), runtime.calls.end(),
        [operation](const Call& call) {
            return call.operation == operation;
        }));
}

void SingleItemOperations() {
    Runtime add;
    Check(pal4::InventoryTransactions::Add(1u, 3u, add) == 0 &&
              add.words.at(0x8134u) == 9u &&
              add.calls == std::vector<Call>{
                  {"database"}, {"item-record", 0x5000u, 1u},
                  {"read", 0x6090u}, {"inventory"},
                  {"read", 0x8134u}, {"inventory"},
                  {"counter", 0x8000u, 0xffffffebu},
                  {"inventory"}, {"process", 0x8000u, 1u, 3u}},
          "single add checks signed balance then pays and processes quantity");

    Runtime missing_add;
    Check(pal4::InventoryTransactions::Add(3u, 1u, missing_add) == 0 &&
              missing_add.calls == std::vector<Call>{
                  {"database"}, {"item-record", 0x5000u, 3u}},
          "missing item record returns before inventory access");
    Runtime expensive;
    expensive.words[0x8134u] = 20u;
    Check(pal4::InventoryTransactions::Add(1u, 3u, expensive) == 45 &&
              Count(expensive, "counter") == 0u &&
              Count(expensive, "process") == 0u,
          "insufficient balance prevents single add mutation");
    Runtime signed_total;
    signed_total.words[0x6090u] = 0xffffffffu;
    signed_total.words[0x8134u] = 0u;
    Check(pal4::InventoryTransactions::Add(1u, 1u, signed_total) == 0 &&
              signed_total.words[0x8134u] == 1u,
          "signed comparison and negation preserve 32-bit price bits");

    Runtime remove;
    Check(pal4::InventoryTransactions::Remove(1u, 2u, remove) == 0 &&
              remove.words.at(0x8134u) == 34u &&
              remove.words.at(0x9008u) == 2u &&
              remove.calls == std::vector<Call>{
                  {"inventory"}, {"entry", 0x8000u, 1u, 5u},
                  {"read", 0x9008u}, {"read", 0x9000u},
                  {"read", 0xa090u}, {"inventory"},
                  {"counter", 0x8000u, 4u},
                  {"read", 0x9000u}, {"read", 0xa000u},
                  {"inventory"}, {"remove", 0x8000u, 1u, 2u}},
          "single remove credits integer quarter price before removal");

    Runtime missing_remove;
    Check(pal4::InventoryTransactions::Remove(3u, 1u, missing_remove) == 0 &&
              missing_remove.calls.size() == 2u,
          "missing inventory entry returns without mutation");
    Runtime too_many;
    Check(pal4::InventoryTransactions::Remove(1u, 5u, too_many) == 46 &&
              Count(too_many, "counter") == 0u,
          "quantity greater than stock returns 46");
    Runtime negative_price;
    negative_price.words[0xa090u] = 0xfffffff7u;
    Check(pal4::InventoryTransactions::Remove(1u, 2u, negative_price) == 0 &&
              negative_price.calls[6] ==
                  Call{"counter", 0x8000u, 0xfffffffcu},
          "signed price divides toward zero before quantity multiplication");
}

void BatchOperations() {
    constexpr std::array<pal4::ItemQuantity, 2> items{{
        {1u, 2u}, {2u, 1u}}};
    Runtime add;
    Check(pal4::InventoryTransactions::AddMultiple(items, add) == 0 &&
              add.words.at(0x8134u) == 5u &&
              Count(add, "process") == 2u &&
              Count(add, "item-record") == 4u,
          "batch add validates cumulative cost then invokes both single adds");
    const auto first_counter = std::find_if(
        add.calls.begin(), add.calls.end(), [](const Call& call) {
            return call.operation == "counter";
        });
    Check(first_counter != add.calls.end() &&
              std::count_if(add.calls.begin(), first_counter,
                            [](const Call& call) {
                                return call.operation == "item-record";
                            }) == 3,
          "both records are prechecked before first batch mutation");

    Runtime missing_add;
    constexpr std::array<pal4::ItemQuantity, 2> missing_items{{
        {1u, 1u}, {3u, 1u}}};
    Check(pal4::InventoryTransactions::AddMultiple(
              missing_items, missing_add) == 0 &&
              Count(missing_add, "counter") == 0u,
          "missing batch item stops preflight without mutation");
    Runtime short_balance;
    short_balance.words[0x8134u] = 20u;
    Check(pal4::InventoryTransactions::AddMultiple(
              items, short_balance) == 45 &&
              Count(short_balance, "counter") == 0u,
          "cumulative batch price is checked before any add");

    Runtime remove;
    Check(pal4::InventoryTransactions::RemoveMultiple(items, remove) == 0 &&
              remove.words.at(0x9008u) == 2u &&
              remove.words.at(0x9108u) == 2u &&
              Count(remove, "remove") == 2u &&
              Count(remove, "entry") == 4u,
          "batch remove preflights both records and invokes single removes");
    Runtime missing_remove;
    Check(pal4::InventoryTransactions::RemoveMultiple(
              missing_items, missing_remove) == 0 &&
              Count(missing_remove, "remove") == 0u,
          "missing batch inventory entry prevents mutation");
    Runtime insufficient;
    constexpr std::array<pal4::ItemQuantity, 1> too_many{{{1u, 5u}}};
    Check(pal4::InventoryTransactions::RemoveMultiple(
              too_many, insufficient) == 46 &&
              Count(insufficient, "remove") == 0u,
          "insufficient batch quantity returns 46 during preflight");

    Runtime duplicates;
    constexpr std::array<pal4::ItemQuantity, 2> repeated{{
        {1u, 3u}, {1u, 3u}}};
    Check(pal4::InventoryTransactions::RemoveMultiple(
              repeated, duplicates) == 0 &&
              Count(duplicates, "remove") == 1u &&
              duplicates.words.at(0x9008u) == 1u,
          "batch ignores a later single-remove failure after duplicate precheck");

    Runtime empty;
    Check(pal4::InventoryTransactions::AddMultiple({}, empty) == 0 &&
              pal4::InventoryTransactions::RemoveMultiple({}, empty) == 0 &&
              empty.calls.empty(),
          "empty batch makes no runtime calls");
}

void RecipePaths() {
    Runtime success;
    Check(pal4::InventoryTransactions::LearnRecipe(0x1234u, success) == 0 &&
              success.words.at(0x8134u) == 5u && success.recipe_known &&
              success.calls == std::vector<Call>{
                  {"database"}, {"recipe-record", 0x5080u, 0x1234u},
                  {"inventory"}, {"has-recipe", 0x8104u, 0x1234u},
                  {"inventory"}, {"read", 0x7078u},
                  {"read", 0x8134u}, {"inventory"},
                  {"counter", 0x8000u, 0xffffffe7u},
                  {"read", 0x7064u}, {"inventory"},
                  {"learn", 0x8104u, 0xbeefu, 0x1234u}},
          "recipe purchase checks ownership and balance before learning");

    Runtime missing;
    missing.recipe_record = 0u;
    Check(pal4::InventoryTransactions::LearnRecipe(0x1234u, missing) == 0 &&
              missing.calls.size() == 2u,
          "missing recipe record does not access inventory");
    Runtime known;
    known.recipe_known = true;
    Check(pal4::InventoryTransactions::LearnRecipe(0x1234u, known) == 0 &&
              known.calls.size() == 4u &&
              Count(known, "counter") == 0u,
          "known recipe is not purchased again");
    Runtime short_balance;
    short_balance.words[0x8134u] = 24u;
    Check(pal4::InventoryTransactions::LearnRecipe(
              0x1234u, short_balance) == 45 &&
              Count(short_balance, "learn") == 0u,
          "insufficient funds prevent recipe learning");
}

}  // namespace

int main() {
    try {
        SingleItemOperations();
        BatchOperations();
        RecipePaths();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
