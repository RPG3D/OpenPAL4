#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowInventoryCollectionRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t input = 0x1234u;
    static constexpr std::uint32_t db_item = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0xbcu, 0x8000u},
        {object + 0xc0u, 0x8008u},
        {object + 0xc4u, 0x8008u},
        {0x8000u, 98u}, {0x8004u, 99u},
        {db_item + 0x94u, 5u},
        {0x6000u + 0x90u, 77u},
        {0x6000u + 0x190u, 111u},
        {0x6400u + 0x90u, 88u},
        {0x6400u + 0x190u, 999u},
        {0x6800u + 0x90u, 77u},
        {0x6800u + 0x190u, 222u},
        {0x6c00u + 0x90u, 77u},
        {0x6c00u + 0x190u, 333u},
    };
    std::array<std::uint32_t, 4> records{
        0x6000u, 0x6400u, 0x6800u, 0x6c00u};
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::string> calls;
    std::int32_t resolved_id = 77;
    std::int32_t inventory_count = 4;
    bool has_db_item = true;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    std::uint32_t AllocateBytes(std::uint32_t size) override {
        allocations.push_back(size);
        calls.emplace_back("allocate");
        return 0x9000u;
    }
    void FreeBytes(std::uint32_t address) override {
        frees.push_back(address);
        calls.emplace_back("free");
    }
    std::int32_t ResolveSmithItemId(std::uint32_t value) override {
        Check(value == input, "Smith item lookup input");
        calls.emplace_back("resolve");
        return resolved_id;
    }
    bool HasDatabaseItem(std::uint32_t id) override {
        Check(id == 77u, "database item ID");
        calls.emplace_back("has-db-item");
        return has_db_item;
    }
    std::uint32_t DatabaseItem(std::uint32_t id) override {
        Check(id == 77u, "database item lookup ID");
        calls.emplace_back("db-item");
        return db_item;
    }
    std::int32_t InventorySize(std::uint32_t category) override {
        Check(category == 5u, "inventory category");
        calls.emplace_back("inventory-size");
        return inventory_count;
    }
    std::uint32_t InventoryAt(std::uint32_t category,
                              std::uint32_t index) override {
        Check(category == 5u && index < records.size(),
              "inventory category and index");
        calls.emplace_back("inventory-at");
        return records[index];
    }
};

void CheckMatchingRecordsAndGrowth() {
    Runtime runtime;
    Check(pal4::SmithWindow::CollectMatchingInventoryItemIds(
              Runtime::object, Runtime::input, runtime) == 4u &&
              runtime.words.at(Runtime::object + 0xbcu) == 0x9000u &&
              runtime.words.at(Runtime::object + 0xc0u) == 0x900cu &&
              runtime.words.at(Runtime::object + 0xc4u) == 0x9010u &&
              runtime.words.at(0x9000u) == 111u &&
              runtime.words.at(0x9004u) == 222u &&
              runtime.words.at(0x9008u) == 333u &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{16u}) &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x8000u}),
          "matching inventory IDs append and grow the 32-bit array");
    Check(runtime.calls.front() == "read" &&
              runtime.calls.at(1) == "write" &&
              runtime.calls.at(2) == "resolve",
          "old selection clears before Smith ID resolution");
}

void CheckEarlyExits() {
    Runtime missing_mapping;
    missing_mapping.resolved_id = -1;
    Check(pal4::SmithWindow::CollectMatchingInventoryItemIds(
              Runtime::object, Runtime::input,
              missing_mapping) == 0xffu &&
              missing_mapping.words.at(Runtime::object + 0xc0u) ==
                  0x8000u &&
              missing_mapping.calls.back() == "resolve",
          "missing Smith mapping returns after clearing array");

    Runtime missing_db;
    missing_db.has_db_item = false;
    Check(pal4::SmithWindow::CollectMatchingInventoryItemIds(
              Runtime::object, Runtime::input, missing_db) == 0u &&
              missing_db.words.at(Runtime::object + 0xc0u) ==
                  0x8000u &&
              missing_db.calls.back() == "has-db-item",
          "missing GameDB item skips inventory");

    Runtime no_inventory;
    no_inventory.inventory_count = 0;
    Check(pal4::SmithWindow::CollectMatchingInventoryItemIds(
              Runtime::object, Runtime::input,
              no_inventory) == 0u &&
              no_inventory.words.at(Runtime::object + 0xc0u) ==
                  0x8000u &&
              no_inventory.allocations.empty(),
          "empty inventory keeps the array empty");

    Runtime negative_count;
    negative_count.inventory_count = -2;
    Check(pal4::SmithWindow::CollectMatchingInventoryItemIds(
              Runtime::object, Runtime::input,
              negative_count) == 0xfeu,
          "negative inventory count forwards its low byte");
}

}  // namespace

int main() {
    try {
        CheckMatchingRecordsAndGrowth();
        CheckEarlyExits();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
