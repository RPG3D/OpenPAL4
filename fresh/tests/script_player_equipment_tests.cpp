#include "pal4/script_player_equipment.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerEquipmentRuntime {
    std::uint32_t player = 0x2000u;
    std::uint8_t valid = 1u;
    std::uint32_t category = 2u;
    pal4::InventoryItemSlotsState slots;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return 0x1000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x1000u && index == 1,
              "player lookup operands");
        calls.emplace_back("player"); return player;
    }
    std::uint32_t GetGameDatabase() override {
        calls.emplace_back("database"); return 0x3000u;
    }
    std::uint8_t IsEquipment(std::uint32_t table,
                             std::uint32_t id) override {
        Check(table == 0x3058u && id == 0x66u,
              "equipment table +0x58 and ID");
        calls.emplace_back("validate"); return valid;
    }
    std::uint32_t GetInventory() override {
        calls.emplace_back("inventory"); return 0x4000u;
    }
    std::uint32_t ProcessInventoryCollection(
        std::uint32_t collection, std::uint32_t id) override {
        Check(collection == 0x407cu && id == 0x66u,
              "special inventory collection +0x7c");
        calls.emplace_back("record"); return 0x5000u;
    }
    pal4::InventoryItemSlotsState ReadEquipmentSlots(
        std::uint32_t address) override {
        Check(address == player + 0x920u,
              "six equipment slots start at player +0x920");
        calls.emplace_back("slots"); return slots;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        if (address == 0x5190u) return 0x66u;
        if (address == 0x5094u) return 0x77u;
        throw std::runtime_error("wrong inventory record field");
    }
    std::uint32_t ProcessGameDatabaseArray(
        std::uint32_t table, std::uint32_t value) override {
        Check(table == 0x3058u && value == 0x77u,
              "slot classifier database source");
        calls.emplace_back("classify"); return category;
    }
    std::uint32_t FindInventoryEntry(std::uint32_t, std::uint32_t,
                                     std::uint32_t) override {
        throw std::runtime_error("unexpected inventory lookup");
    }
    std::uint32_t ProcessEntry(std::uint32_t, std::uint32_t,
                               std::uint32_t) override {
        throw std::runtime_error("unexpected entry process");
    }
    void InsertEntry(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected entry insertion");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address >= player + 0x920u &&
              address <= player + 0x934u &&
              (address - player - 0x920u) % 4u == 0u && value == 0x66u,
              "equipment ID written only to slot zero through five");
        calls.emplace_back("write");
        writes.emplace_back(address, value);
    }
    std::uint8_t RefreshEquipmentState(
        std::uint32_t state) override {
        Check(state == player + 0x654u,
              "player state virtual slot +0x114");
        calls.emplace_back("refresh"); return 0xa5u;
    }
};

void Gates() {
    Runtime negative;
    Check(pal4::ScriptPlayerEquipment::Add(-2, 0x66u, negative) == 0xfeu &&
          negative.calls.empty(), "negative index returns low input byte");

    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptPlayerEquipment::Add(1, 0x66u, missing) == 0u &&
          missing.calls == std::vector<std::string>{"manager", "player"},
          "missing player exits before database lookup");

    Runtime invalid;
    invalid.valid = 0u;
    Check(pal4::ScriptPlayerEquipment::Add(1, 0x66u, invalid) == 0u &&
          invalid.calls == std::vector<std::string>{
              "manager", "player", "database", "validate"},
          "invalid equipment exits before inventory access");
}

void SlotCases() {
    for (const std::uint32_t slot : {0u, 1u, 2u, 3u}) {
        Runtime runtime;
        runtime.category = slot;
        Check(pal4::ScriptPlayerEquipment::Add(1, 0x66u, runtime) == 0xa5u &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {runtime.player + 0x920u + slot * 4u, 0x66u}} &&
              runtime.calls.back() == "refresh",
              "classifier chooses ordinary slot and refreshes state");
    }

    Runtime existing;
    existing.slots.ids[1] = 0x66u;
    Check(pal4::ScriptPlayerEquipment::Add(1, 0x66u, existing) == 0xa5u &&
          existing.writes.front().first == existing.player + 0x924u,
          "matching ID reuses its existing slot");
    for (const auto& call : existing.calls)
        Check(call != "classify", "matching ID bypasses classification");

    Runtime first_special;
    first_special.category = 4u;
    Check(pal4::ScriptPlayerEquipment::Add(1, 0x66u,
                                           first_special) == 0xa5u &&
          first_special.writes.front().first ==
              first_special.player + 0x930u,
          "first special equipment uses slot four");

    Runtime second_special;
    second_special.category = 4u;
    second_special.slots.ids[4] = 0x88u;
    Check(pal4::ScriptPlayerEquipment::Add(1, 0x66u,
                                           second_special) == 0xa5u &&
          second_special.writes.front().first ==
              second_special.player + 0x934u,
          "occupied slot four selects empty slot five");

    Runtime unclassified;
    unclassified.category = 5u;
    Check(pal4::ScriptPlayerEquipment::Add(1, 0x66u,
                                           unclassified) == 0xa5u &&
          unclassified.writes.empty() &&
          unclassified.calls.back() == "refresh",
          "out-of-range classification still refreshes player state");
}

}  // namespace

int main() {
    try {
        Gates();
        SlotCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
