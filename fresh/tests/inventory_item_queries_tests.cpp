#include "pal4/inventory_item_queries.h"

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

struct Runtime final : pal4::IInventoryItemQueryRuntime {
    static constexpr std::uint32_t inventory = 0x1000u;
    static constexpr std::uint32_t entry = 0x2000u;
    static constexpr std::uint32_t destination = 0x3000u;
    static constexpr std::uint32_t source = entry + 0x98u;
    static constexpr std::uint32_t category = 0x4000u;
    static constexpr std::uint32_t recipe = 0x7000u;
    static constexpr std::uint32_t empty = 0x9001u;

    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> elements;
    bool allocation_succeeds = true;
    std::uint32_t found_entry = entry;
    std::uint32_t found_db_entry = entry;
    std::uint32_t found_category = category;
    std::int32_t count_override = -1;
    int inventory_calls = 0;

    Runtime() {
        bytes[source] = 3u;
        words[source + 4u] = 0x8001u;
        words[source + 8u] = 3u;
        words[source + 12u] = 7u;
        words[destination + 16u] = 0xbeefu;
        words[entry + 0x184u] = 0x4321u;
        words[entry + 0x17cu] = 4u;
        words[entry + 0x180u] = 9u;
        words[entry + 8u] = 0x5678u;
        words[category + 0x94u] = 12u;
        words[recipe + 0x6cu] = 5u;
        words[recipe + 0x70u] = 3u;
        words[recipe + 0x74u] = 9u;
        bytes[0x8000u] = 1u;
        bytes[0x8001u] = 'a';
        bytes[0x8002u] = 'b';
        bytes[0x8003u] = 'c';
        bytes[empty - 1u] = 0xffu;
    }

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t EmptyText() override { return empty; }
    void ThrowRange() override { throw std::out_of_range("text range"); }
    void DetachAndReset(std::uint32_t record) override {
        calls.push_back("reset");
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        calls.push_back("allocate " + std::to_string(length) +
                        " " + std::to_string(mode));
        if (!allocation_succeeds) return false;
        words[record + 4u] = 0xa001u;
        return true;
    }
    void SetTextSize(std::uint32_t record, std::uint32_t length) override {
        calls.push_back("size " + std::to_string(length));
        words[record + 8u] = length;
    }
    void SetErrorFlag(std::uint32_t, std::uint8_t flag) override {
        calls.push_back("flag " + std::to_string(flag));
    }
    void CopyBytes(std::uint32_t target, std::uint32_t from,
                   std::uint32_t length) override {
        calls.push_back("copy " + std::to_string(length));
        for (std::uint32_t i = 0; i < length; ++i)
            bytes[target + i] = bytes.at(from + i);
    }
    std::uint32_t InventorySubsystem() override {
        ++inventory_calls;
        calls.push_back("inventory");
        return inventory;
    }
    std::uint32_t FindInventoryEntry(std::uint32_t collection,
                                     std::uint32_t key,
                                     std::uint32_t kind) override {
        Check(collection == inventory + 0x7cu && key == 42u && kind == 7u,
              "inventory entry lookup arguments");
        calls.push_back("entry");
        return found_entry;
    }
    std::uint32_t FindGameDbEntry(std::uint32_t owner,
                                  std::uint32_t key,
                                  std::uint32_t kind) override {
        Check(owner == inventory && key == 42u && kind == 5u,
              "game database entry lookup arguments");
        calls.push_back("db entry");
        return found_db_entry;
    }
    std::uint32_t GameDb() override {
        calls.push_back("game db");
        return 0x5000u;
    }
    std::uint32_t FindCategory(std::uint32_t collection,
                               std::uint32_t key) override {
        Check(collection == 0x5058u && key == 42u,
              "category lookup arguments");
        calls.push_back("category");
        return found_category;
    }
    std::uint32_t FindRecipe(std::uint32_t collection,
                             std::uint32_t key) override {
        Check(collection == 0x5080u && key == 88u,
              "recipe lookup arguments");
        calls.push_back("recipe");
        return recipe;
    }
    std::int32_t ElementCount(std::uint32_t collection,
                              std::uint32_t kind) override {
        Check(collection == inventory + 0x7cu && kind == 12u,
              "element count arguments");
        calls.push_back("count");
        return count_override < 0 ? static_cast<std::int32_t>(elements.size())
                                  : count_override;
    }
    std::uint32_t ElementAt(std::uint32_t collection,
                            std::uint32_t kind,
                            std::int32_t index) override {
        Check(collection == inventory + 0x7cu && kind == 12u,
              "element index arguments");
        calls.push_back("at " + std::to_string(index));
        return elements.at(static_cast<std::size_t>(index));
    }
};

void CheckTextCopy() {
    Runtime shared;
    Check(pal4::InventoryItemQueries::CopyItemData(
              shared.destination, 42u, shared) == shared.destination,
          "copy returns destination");
    Check(shared.calls == (std::vector<std::string>{
              "inventory", "entry", "flag 1"}) &&
              shared.bytes.at(shared.destination) == 3u &&
              shared.words.at(shared.destination + 4u) == 0x8001u &&
              shared.words.at(shared.destination + 8u) == 3u &&
              shared.words.at(shared.destination + 12u) == 7u &&
              shared.words.at(shared.destination + 16u) == 0xbeefu &&
              shared.bytes.at(0x8000u) == 2u,
          "shared item text preserves unrelated tail word");

    Runtime owned;
    owned.bytes[0x8000u] = 0xfeu;
    pal4::InventoryItemQueries::CopyItemData(owned.destination, 42u, owned);
    Check(owned.calls == (std::vector<std::string>{
              "inventory", "entry", "allocate 3 1", "copy 3"}) &&
              owned.bytes.at(0xa001u) == 'a' &&
              owned.bytes.at(0xa003u) == 'c' &&
              owned.bytes.at(0xa004u) == 0u &&
              owned.words.at(owned.destination + 8u) == 3u,
          "saturated item text is copied and terminated");

    Runtime failed;
    failed.allocation_succeeds = false;
    failed.bytes[0x8000u] = 0xfeu;
    pal4::InventoryItemQueries::CopyItemData(failed.destination, 42u, failed);
    Check(failed.words.at(failed.destination + 4u) == 0u &&
              failed.words.at(failed.destination + 8u) == 0u &&
              failed.words.at(failed.destination + 16u) == 0xbeefu &&
              failed.calls.back() == "allocate 3 1",
          "allocation failure leaves initialized record");

    Runtime self;
    self.found_entry = self.destination - 0x98u;
    self.bytes[self.destination] = 9u;
    self.words[self.destination + 8u] = 4u;
    bool threw = false;
    try {
        pal4::InventoryItemQueries::CopyItemData(
            self.destination, 42u, self);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Check(!threw && self.calls == (std::vector<std::string>{
              "inventory", "entry", "reset", "reset"}) &&
              self.words.at(self.destination + 8u) == 0u,
          "self-copy zeros size before range check and resets twice");
}

void CheckSimpleQueries() {
    Runtime runtime;
    Check(pal4::InventoryItemQueries::GetProperty(42u, runtime) == 0x4321u &&
              runtime.calls == (std::vector<std::string>{
                  "inventory", "entry"}),
          "item property is entry word 0x184");
    runtime.calls.clear();
    Check(pal4::InventoryItemQueries::FindGameDbValue(42u, runtime) ==
              0x5678u &&
              runtime.calls == (std::vector<std::string>{
                  "inventory", "db entry"}),
          "database result uses entry word eight");
    runtime.found_db_entry = 0u;
    Check(pal4::InventoryItemQueries::FindGameDbValue(42u, runtime) == 0u,
          "missing database entry yields zero");
}

void CheckCounting() {
    Runtime missing;
    missing.found_category = 0u;
    Check(pal4::InventoryItemQueries::CountMatching(42u, missing) == 0 &&
              missing.inventory_calls == 0 &&
              missing.calls == (std::vector<std::string>{
                  "game db", "category"}),
          "missing category stops before inventory lookup");

    Runtime mixed;
    mixed.elements = {0x6000u, 0u, 0x6100u, 0x6200u};
    mixed.words[0x6000u + 0x90u] = 42u;
    mixed.words[0x6100u + 0x90u] = 7u;
    mixed.words[0x6200u + 0x90u] = 42u;
    Check(pal4::InventoryItemQueries::CountMatching(42u, mixed) == 2 &&
              mixed.inventory_calls == 5 &&
              mixed.calls == (std::vector<std::string>{
                  "game db", "category", "inventory", "count",
                  "inventory", "at 0", "inventory", "at 1",
                  "inventory", "at 2", "inventory", "at 3"}),
          "count traverses all elements and reloads inventory each time");

    Runtime zero;
    zero.count_override = 0;
    Check(pal4::InventoryItemQueries::CountMatching(42u, zero) == 0 &&
              zero.inventory_calls == 1,
          "zero element count skips loop");

    Runtime threshold;
    threshold.elements.assign(99u, 0x6000u);
    threshold.words[0x6000u + 0x90u] = 42u;
    threshold.count_override = 98;
    Check(!pal4::InventoryItemQueries::HasQuantityLimit(42u, threshold),
          "98 matching entries are below limit");
    threshold.count_override = 99;
    Check(pal4::InventoryItemQueries::HasQuantityLimit(42u, threshold),
          "99 matching entries reach limit");
}

void CheckRecipeProperties() {
    Runtime runtime;
    runtime.words[runtime.entry + 0x184u] = 4u;
    Check(pal4::InventoryItemQueries::IsBelowRecipeQuantity(
              88u, 42u, runtime) &&
              runtime.calls == (std::vector<std::string>{
                  "inventory", "entry", "game db", "recipe"}),
          "quantity subtract sign reports shortage");
    runtime.words[runtime.entry + 0x184u] = 5u;
    Check(!pal4::InventoryItemQueries::IsBelowRecipeQuantity(
              88u, 42u, runtime),
          "equal quantity is not below");
    runtime.words[runtime.entry + 0x184u] = 0x7fffffffu;
    runtime.words[runtime.recipe + 0x6cu] = 0xffffffffu;
    Check(pal4::InventoryItemQueries::IsBelowRecipeQuantity(
              88u, 42u, runtime),
          "subtraction sign bit is observed even on signed overflow");

    Runtime property;
    Check(pal4::InventoryItemQueries::MeetsRecipeProperty(
              88u, 42u, property),
          "inventory property four meets required three");
    property.words[property.entry + 0x17cu] = 0xffffffffu;
    Check(!pal4::InventoryItemQueries::MeetsRecipeProperty(
              88u, 42u, property),
          "property comparison is signed");
    Check(pal4::InventoryItemQueries::HasItemProperty(42u, property) &&
              pal4::InventoryItemQueries::MatchesRecipeProperty(
                  42u, 88u, property),
          "nonzero property matches recipe word 0x74");
    property.words[property.recipe + 0x74u] = 8u;
    Check(!pal4::InventoryItemQueries::MatchesRecipeProperty(
              42u, 88u, property),
          "different property does not match");
    property.calls.clear();
    property.words[property.entry + 0x180u] = 0u;
    Check(!pal4::InventoryItemQueries::HasItemProperty(42u, property) &&
              !pal4::InventoryItemQueries::MatchesRecipeProperty(
                  42u, 88u, property) &&
              property.calls == (std::vector<std::string>{
                  "inventory", "entry", "inventory", "entry"}),
          "zero property avoids GameDB lookup");
}

}  // namespace

int main() {
    try {
        CheckTextCopy();
        CheckSimpleQueries();
        CheckCounting();
        CheckRecipeProperties();
        std::cout << "inventory_item_queries_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
