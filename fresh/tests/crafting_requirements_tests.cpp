#include "pal4/crafting_requirements.h"

#include <array>
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

struct Runtime final : pal4::ICraftingRequirementRuntime {
    static constexpr std::uint32_t record = 0x2000u;
    static constexpr std::uint32_t destination = 0x3000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::int32_t> direct_owned;
    std::map<std::uint32_t, std::int32_t> category_owned;
    std::vector<std::string> calls;
    std::uint32_t allocated = 0x8000u;
    std::uint32_t requested_bytes = 0xffffffffu;

    Runtime() {
        Set(0, 10u, 5);
        Set(1, 20u, 3);
        Set(2, 30u, 7);
        Set(3, 40u, 0);
        direct_owned[10u] = 2;
        direct_owned[40u] = 0;
        category_owned[20u] = 1;
        words[record + 0x68u] = 77u;
        words[0x6090u] = 55u;
    }
    void Set(std::uint32_t index, std::uint32_t item,
             std::int32_t required) {
        words[record + 0x98u + index * 4u] = item;
        words[record + 0xa8u + index * 4u] =
            static_cast<std::uint32_t>(required);
    }
    std::uint32_t GameDb() override {
        calls.push_back("db");
        return 0x1000u;
    }
    std::uint32_t FindRecipe(std::uint32_t collection,
                             std::uint32_t recipe) override {
        Check(collection == 0x1080u && recipe == 0x1234u,
              "recipe lookup arguments");
        calls.push_back("recipe");
        return record;
    }
    bool IsDirectItem(std::uint32_t db,
                       std::uint32_t item) override {
        Check(db == 0x1000u, "direct-item database address");
        calls.push_back("direct? " + std::to_string(item));
        return direct_owned.contains(item);
    }
    bool IsCategoryItem(std::uint32_t collection,
                         std::uint32_t item) override {
        Check(collection == 0x1058u,
              "category-item database address");
        calls.push_back("category? " + std::to_string(item));
        return category_owned.contains(item);
    }
    std::uint32_t FindCategoryData(std::uint32_t collection,
                                   std::uint32_t item) override {
        Check(collection == 0x1058u && item == 20u,
              "category data lookup arguments");
        calls.push_back("category data");
        return 0x6000u;
    }
    std::int32_t DirectOwned(std::uint32_t item) override {
        calls.push_back("direct owned " + std::to_string(item));
        return direct_owned.at(item);
    }
    std::int32_t CategoryOwned(std::uint32_t item) override {
        calls.push_back("category owned " + std::to_string(item));
        return category_owned.at(item);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t count) override {
        calls.push_back("allocate");
        requested_bytes = count;
        return allocated;
    }
};

void CheckDeficits() {
    Runtime runtime;
    Check(pal4::CraftingRequirements::BuildDeficits(
              runtime.destination, 0x1234u, runtime) ==
              runtime.destination,
          "builder returns destination");
    Check(runtime.bytes.at(runtime.destination) == 0x34u &&
              runtime.requested_bytes == 16u &&
              runtime.words.at(runtime.destination + 4u) == 0x8000u &&
              runtime.words.at(runtime.destination + 8u) == 0x8010u &&
              runtime.words.at(runtime.destination + 12u) == 0x8010u &&
              runtime.words.at(0x8000u) == 10u &&
              runtime.words.at(0x8004u) == 3u &&
              runtime.words.at(0x8008u) == 20u &&
              runtime.words.at(0x800cu) == 2u,
          "two deficits use eight-byte pairs in recipe order");
    Check(runtime.calls.front() == "db" &&
              runtime.calls.at(1) == "recipe" &&
              runtime.calls.back() == "allocate" &&
              runtime.calls.at(2) == "read 8360" &&
              runtime.calls.at(3) == "read 8344",
          "recipe lookup and first quantity/id reads keep IDA order");

    Runtime empty;
    empty.direct_owned[10u] = 5;
    empty.category_owned[20u] = 3;
    empty.allocated = 0u;
    pal4::CraftingRequirements::BuildDeficits(
        empty.destination, 0x1234u, empty);
    Check(empty.requested_bytes == 0u &&
              empty.words.at(empty.destination + 4u) == 0u &&
              empty.words.at(empty.destination + 8u) == 0u,
          "satisfied recipe still requests zero-byte allocation");

    Runtime failed;
    failed.allocated = 0u;
    pal4::CraftingRequirements::BuildDeficits(
        failed.destination, 0x1234u, failed);
    Check(failed.requested_bytes == 16u &&
              failed.words.at(failed.destination + 4u) == 0u &&
              failed.words.at(failed.destination + 8u) == 16u &&
              failed.words.at(failed.destination + 12u) == 16u &&
              failed.words.find(0x8000u) == failed.words.end(),
          "allocation failure retains observed pointer arithmetic without writes");
}

void CheckRequirementGate() {
    Runtime first_missing;
    Check(!pal4::CraftingRequirements::AreMet(0x1234u, first_missing) &&
              first_missing.calls.back() == "direct owned 10" &&
              first_missing.calls.size() == 7u,
          "first shortage returns before remaining slots");

    Runtime category_missing;
    category_missing.direct_owned[10u] = 5;
    Check(!pal4::CraftingRequirements::AreMet(0x1234u,
                                              category_missing) &&
              category_missing.calls.back() == "category owned 20",
          "category shortage returns false");

    Runtime satisfied;
    satisfied.direct_owned[10u] = 6;
    satisfied.category_owned[20u] = 3;
    Check(pal4::CraftingRequirements::AreMet(0x1234u, satisfied) &&
              satisfied.calls.back() == "read 8356",
          "unknown category is skipped and nonpositive fourth quantity ignored");

    Runtime negative;
    negative.Set(0, 10u, -1);
    negative.Set(1, 20u, 0);
    Check(pal4::CraftingRequirements::AreMet(0x1234u, negative),
          "nonpositive signed requirements are skipped");
}

void CheckRecipeQueries() {
    Runtime runtime;
    Check(pal4::CraftingRequirements::HasCategoryItem(0x1234u, runtime) &&
              runtime.calls.back() == "category? 20",
          "first positive category item is found");
    runtime.calls.clear();
    Check(pal4::CraftingRequirements::FirstCategoryValue(
              0x1234u, runtime) == 55 &&
              runtime.calls.at(runtime.calls.size() - 2u) ==
                  "category data" &&
              runtime.calls.back() == "read 24720",
          "first category lookup reads its word 0x90");
    runtime.calls.clear();
    Check(pal4::CraftingRequirements::GetRecipeProperty(
              0x1234u, runtime) == 77u &&
              runtime.calls.size() == 3u,
          "recipe property uses word 0x68");

    Runtime missing;
    missing.category_owned.clear();
    Check(!pal4::CraftingRequirements::HasCategoryItem(
              0x1234u, missing) &&
              pal4::CraftingRequirements::FirstCategoryValue(
                  0x1234u, missing) == -1,
          "four misses return false and minus one");
    Runtime nonpositive;
    nonpositive.Set(1, 20u, -3);
    Check(!pal4::CraftingRequirements::HasCategoryItem(
              0x1234u, nonpositive),
          "nonpositive category quantity is ignored");
}

}  // namespace

int main() {
    try {
        CheckDeficits();
        CheckRequirementGate();
        CheckRecipeQueries();
        std::cout << "crafting_requirements_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
