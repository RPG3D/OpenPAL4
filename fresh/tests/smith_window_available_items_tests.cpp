#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowAvailableItemsRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t source = 0x3000u;
    static constexpr std::uint32_t recipe = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0xccu, 0x8000u},
        {object + 0xd0u, 0x8008u},
        {object + 0xd4u, 0x8010u},
        {0x8000u, 99u}, {0x8004u, 98u},
        {source, 42u},
        {recipe + 0x7cu, static_cast<std::uint32_t>(-1)},
        {recipe + 0x80u, 3u},
        {recipe + 0x84u, 2u},
        {recipe + 0x88u, 0u},
        {recipe + 0x8cu, 5u},
        {recipe + 0x90u, 1u},
        {recipe + 0x94u, 1u},
    };
    std::array<std::int32_t, 7> inventory_sizes{
        2, 3, 1, 4, 0, 2, -1};
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::uint32_t> queried_categories;
    std::vector<std::string> calls;
    std::uint32_t updated_category = 0xffffffffu;
    bool has_recipe = true;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
        calls.emplace_back("write");
    }
    std::uint32_t AllocateBytes(std::uint32_t) override {
        throw std::runtime_error("unexpected category allocation");
    }
    void FreeBytes(std::uint32_t) override {
        throw std::runtime_error("unexpected category free");
    }
    bool HasRecipe(std::uint32_t id) override {
        Check(id == 42u, "source recipe ID");
        calls.emplace_back("has-recipe");
        return has_recipe;
    }
    std::uint32_t RecipeForId(std::uint32_t id) override {
        Check(id == 42u, "recipe lookup ID");
        calls.emplace_back("recipe");
        return recipe;
    }
    std::int32_t InventorySize(std::uint32_t category) override {
        queried_categories.push_back(category);
        calls.emplace_back("inventory-size");
        return inventory_sizes.at(category);
    }
    std::uint32_t UpdateEquipmentList(std::uint32_t owner,
                                       std::uint32_t category) override {
        Check(owner == object, "equipment list owner");
        updated_category = category;
        calls.emplace_back("update-list");
        return 0xabcd1234u;
    }
};

void CheckAvailableCategories() {
    Runtime runtime;
    Check(pal4::SmithWindow::UpdateAvailableEquipmentCategories(
              Runtime::object, Runtime::source, runtime) == 0x34u &&
              runtime.writes.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{
                      Runtime::object + 0xd0u, 0x8000u} &&
              runtime.words.at(0x8000u) == 1u &&
              runtime.words.at(0x8004u) == 2u &&
              runtime.words.at(0x8008u) == 5u &&
              runtime.queried_categories ==
                  (std::vector<std::uint32_t>{1u, 2u, 4u, 5u, 6u}) &&
              runtime.updated_category == 1u &&
              runtime.words.at(Runtime::object + 0xb0u) == 0u &&
              runtime.words.at(Runtime::object + 0xd0u) == 0x800cu,
          "seven-category filtering, insertion and first selection");
    Check(runtime.calls.front() == "read" &&
              runtime.calls.at(1) == "write" &&
              runtime.calls.back() == "update-list",
          "old category range clears before refill");
}

void CheckMissingAndEmpty() {
    Runtime missing;
    missing.has_recipe = false;
    Check(pal4::SmithWindow::UpdateAvailableEquipmentCategories(
              Runtime::object, Runtime::source, missing) == 0u &&
              missing.writes.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{
                      Runtime::object + 0xd0u, 0x8000u} &&
              missing.words.at(Runtime::object + 0xd0u) == 0x8000u &&
              !missing.words.contains(Runtime::object + 0xb0u),
          "missing recipe returns after clearing prior range");

    Runtime empty;
    empty.words[Runtime::object + 0xccu] = 0x8034u;
    empty.words[Runtime::object + 0xd0u] = 0x8034u;
    empty.words[Runtime::object + 0xd4u] = 0x8034u;
    empty.inventory_sizes.fill(0);
    Check(pal4::SmithWindow::UpdateAvailableEquipmentCategories(
              Runtime::object, Runtime::source, empty) == 0x34u &&
              empty.words.at(Runtime::object + 0xb0u) == 0xffffffffu &&
              empty.updated_category == 0xffffffffu,
          "empty available array keeps invalid selection and begin low byte");
}

}  // namespace

int main() {
    try {
        CheckAvailableCategories();
        CheckMissingAndEmpty();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
