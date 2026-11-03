#include "pal4/inventory_item_slots.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IInventoryItemSlotsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t category = 0u;
    std::uint32_t processed = 0u;

    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t GetGameDatabase() override {
        calls.push_back("database");
        return 0x2000u;
    }
    std::uint32_t ProcessGameDatabaseArray(
        std::uint32_t array_address, std::uint32_t value) override {
        calls.push_back("classify:" + std::to_string(array_address) + ":" +
                        std::to_string(value));
        return category;
    }
    std::uint32_t GetInventory() override {
        calls.push_back("inventory");
        return 0x3000u;
    }
    std::uint32_t FindInventoryEntry(
        std::uint32_t array_address, std::uint32_t id,
        std::uint32_t category_id) override {
        calls.push_back("find:" + std::to_string(array_address) + ":" +
                        std::to_string(id) + ":" +
                        std::to_string(category_id));
        return id + 0x100u;
    }
    std::uint32_t ProcessEntry(std::uint32_t first_argument,
                               std::uint32_t second_argument,
                               std::uint32_t entry) override {
        calls.push_back("process:" + std::to_string(first_argument) + ":" +
                        std::to_string(second_argument) + ":" +
                        std::to_string(entry));
        return ++processed;
    }
    void InsertEntry(std::uint32_t destination,
                     std::uint32_t entry) override {
        calls.push_back("insert:" + std::to_string(destination) + ":" +
                        std::to_string(entry));
    }
};

void TestInitializationAndMembership() {
    pal4::InventoryItemSlotsState state;
    state.ids.fill(0xa5a5a5a5u);
    Check(pal4::InventoryItemSlots::Initialize(state) == 0u &&
              state.ids == std::array<std::uint32_t, 6>{},
          "initializer clears all six dwords and leaves zero in EAX");
    state.ids = {11u, 22u, 33u, 44u, 55u, 66u};
    Check(pal4::InventoryItemSlots::Contains(state, 11u) &&
              pal4::InventoryItemSlots::Contains(state, 66u) &&
              !pal4::InventoryItemSlots::Contains(state, 77u),
          "membership checks first, last and missing slots");
}

void TestIndexedAccess() {
    pal4::InventoryItemSlotsState state;
    state.ids = {11u, 22u, 33u, 44u, 55u, 66u};
    Check(pal4::InventoryItemSlots::GetIndexed(state, 0) == 11u &&
              pal4::InventoryItemSlots::GetIndexed(state, 5) == 66u &&
              pal4::InventoryItemSlots::GetIndexed(state, -1) ==
                  0xffffffffu &&
              pal4::InventoryItemSlots::GetIndexed(state, 6) == 6u,
          "getter returns slot values or the unchanged invalid index");
    Check(pal4::InventoryItemSlots::SetIndexed(
              state, 0, 101u) == 101u &&
              pal4::InventoryItemSlots::SetIndexed(
                  state, 5, 606u) == 606u &&
              pal4::InventoryItemSlots::SetIndexed(
                  state, -2, 999u) == 0xfffffffeu &&
              pal4::InventoryItemSlots::SetIndexed(
                  state, 6, 999u) == 6u &&
              state.ids == std::array<std::uint32_t, 6>{
                  101u, 22u, 33u, 44u, 55u, 606u},
          "setter writes valid slots and leaves invalid indices untouched");
}

void TestClassification() {
    pal4::InventoryItemSlotsState state;
    state.ids = {11u, 22u, 33u, 44u, 55u, 66u};
    Runtime runtime;
    runtime.words[0x1094u] = 0x99u;
    for (std::uint32_t code = 0u; code < 4u; ++code) {
        runtime.category = code;
        runtime.calls.clear();
        Check(pal4::InventoryItemSlots::Classify(
                  state, 0x1000u, runtime) == code &&
                  runtime.calls == std::vector<std::string>{
                      "read:4244", "database", "classify:8280:153"},
              "first four database categories return unchanged");
    }
    runtime.category = 4u;
    Check(pal4::InventoryItemSlots::Classify(
              state, 0x1000u, runtime) == 4u,
          "category four with two populated tail slots remains four");
    state.ids[5] = 0u;
    Check(pal4::InventoryItemSlots::Classify(
              state, 0x1000u, runtime) == 5u,
          "category four with only slot four populated becomes five");
    state.ids[4] = 0u;
    Check(pal4::InventoryItemSlots::Classify(
              state, 0x1000u, runtime) == 4u,
          "empty fourth slot returns four regardless of the fifth slot");
    runtime.category = 5u;
    Check(pal4::InventoryItemSlots::Classify(
              state, 0x1000u, runtime) == 0x1000u,
          "out-of-range category returns the original record address");
    runtime.category = 0xffffffffu;
    Check(pal4::InventoryItemSlots::Classify(
              state, 0x1000u, runtime) == 0x1000u,
          "unsigned switch also sends negative-like values to default");
}

void TestSlotLookupAndFallback() {
    pal4::InventoryItemSlotsState state;
    state.ids = {11u, 22u, 33u, 44u, 55u, 66u};
    Runtime runtime;
    runtime.words = {{0x1190u, 66u}, {0x1094u, 0x77u}};
    Check(pal4::InventoryItemSlots::FindSlotOrClassify(
              state, 0x1000u, runtime) == 5u &&
              runtime.calls == std::vector<std::string>{"read:4496"},
          "slot hit returns its index without touching the database");
    runtime.words[0x1190u] = 99u;
    runtime.category = 2u;
    runtime.calls.clear();
    Check(pal4::InventoryItemSlots::FindSlotOrClassify(
              state, 0x1000u, runtime) == 2u &&
              runtime.calls == std::vector<std::string>{
                  "read:4496", "read:4244", "database",
                  "classify:8280:119"},
          "slot miss falls through to the database classification path");
}

void TestProcessAllSix() {
    pal4::InventoryItemSlotsState state;
    state.ids = {11u, 22u, 33u, 44u, 55u, 66u};
    Runtime runtime;
    Check(pal4::InventoryItemSlots::ProcessEach(
              state, 0x4000u, 0x5000u, runtime) == 6u &&
              runtime.calls.size() == 18u &&
              runtime.calls[0] == "inventory" &&
              runtime.calls[1] == "find:12412:11:7" &&
              runtime.calls[2] == "process:16384:20480:267" &&
              runtime.calls[15] == "inventory" &&
              runtime.calls[16] == "find:12412:66:7" &&
              runtime.calls[17] == "process:16384:20480:322",
          "each slot acquires inventory, finds category seven and processes it");
}

void TestInsertAllSix() {
    pal4::InventoryItemSlotsState state;
    state.ids = {11u, 22u, 33u, 44u, 55u, 66u};
    Runtime runtime;
    pal4::InventoryItemSlots::ProcessAndInsertEach(
        state, 0x4000u, runtime);
    Check(runtime.calls.size() == 18u &&
              runtime.calls[0] == "inventory" &&
              runtime.calls[1] == "find:12412:11:7" &&
              runtime.calls[2] == "insert:16384:267" &&
              runtime.calls[15] == "inventory" &&
              runtime.calls[16] == "find:12412:66:7" &&
              runtime.calls[17] == "insert:16384:322",
          "insert path resolves each of six category-seven records");
}

}  // namespace

int main() {
    TestInitializationAndMembership();
    TestIndexedAccess();
    TestClassification();
    TestSlotLookupAndFallback();
    TestProcessAllSix();
    TestInsertAllSix();
    pal4::InventoryItemSlotsState state;
    pal4::InventoryItemSlotsRuntimeStub stub;
    Check(pal4::InventoryItemSlots::Classify(state, 0u, stub) == 0u,
          "default inventory item runtime links");
    std::cout << "7 inventory item slots cases passed\n";
}
