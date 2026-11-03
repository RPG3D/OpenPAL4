#include "pal4/script_equipment_inventory_query.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Item {
    std::uint32_t id;
    bool condition;
};

struct Runtime final : pal4::IScriptEquipmentInventoryQueryRuntime {
    bool has_record = true;
    std::uint32_t record = 0x3000u;
    std::int32_t count_override = -999;
    std::vector<Item> items;
    std::vector<std::string> calls;

    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x1000u;
    }
    bool HasRecord(std::uint32_t table, std::int32_t id) override {
        Check(table == 0x1058u && id == -7,
              "first equipment database search operands");
        calls.emplace_back("has");
        return has_record;
    }
    std::uint32_t FindRecord(std::uint32_t table,
                             std::int32_t id) override {
        Check(table == 0x1058u && id == -7,
              "second equipment database search operands");
        calls.emplace_back("record");
        return record;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x3094u) {
            calls.emplace_back("category");
            return 7u;
        }
        if (address == 0x3090u) {
            calls.emplace_back("record-id");
            return 0x55u;
        }
        for (std::size_t index = 0; index < items.size(); ++index) {
            if (address == 0x4090u + index * 0x100u) {
                calls.emplace_back("item-id:" + std::to_string(index));
                return items[index].id;
            }
        }
        throw std::runtime_error("unexpected field offset");
    }
    std::uint32_t Inventory() override {
        calls.emplace_back("inventory");
        return 0x2000u;
    }
    std::int32_t ElementCount(std::uint32_t collection,
                              std::uint32_t category) override {
        Check(collection == 0x207cu && category == 7u,
              "equipment collection and category");
        calls.emplace_back("count");
        return count_override == -999 ?
            static_cast<std::int32_t>(items.size()) : count_override;
    }
    std::uint32_t ElementAt(std::uint32_t collection,
                            std::uint32_t category,
                            std::uint32_t index) override {
        Check(collection == 0x207cu && category == 7u &&
                  index < items.size(), "equipment element lookup operands");
        calls.emplace_back("item:" + std::to_string(index));
        return 0x4000u + index * 0x100u;
    }
    bool CheckPlayersCondition(std::uint32_t element) override {
        const auto index = (element - 0x4000u) / 0x100u;
        Check(index < items.size() &&
                  element == 0x4000u + index * 0x100u,
              "equipment condition receiver");
        calls.emplace_back("condition:" + std::to_string(index));
        return items[index].condition;
    }
};

void EarlyGates() {
    Runtime absent;
    absent.has_record = false;
    Check(!pal4::ScriptEquipmentInventoryQuery::HasUsableEquipment(
              -7, absent) &&
              absent.calls == std::vector<std::string>{"database", "has"},
          "failed first search skips all later work");
    Runtime no_record;
    no_record.record = 0u;
    Check(!pal4::ScriptEquipmentInventoryQuery::HasUsableEquipment(
              -7, no_record) &&
              no_record.calls == std::vector<std::string>{
                  "database", "has", "database", "record"},
          "second search can return null");
    for (auto count : {0, -1}) {
        Runtime empty;
        empty.count_override = count;
        Check(!pal4::ScriptEquipmentInventoryQuery::HasUsableEquipment(
                  -7, empty) &&
                  empty.calls == std::vector<std::string>{
                      "database", "has", "database", "record",
                      "category", "inventory", "count"},
              "nonpositive collection size skips item lookup");
    }
}

void Scan() {
    Runtime hit;
    hit.items = {{0x44u, false}, {0x55u, true}, {0x55u, false},
                 {0x55u, false}};
    Check(pal4::ScriptEquipmentInventoryQuery::HasUsableEquipment(
              -7, hit), "matching element with false condition succeeds");
    Check(hit.calls == std::vector<std::string>{
              "database", "has", "database", "record",
              "category", "inventory", "count",
              "category", "inventory", "item:0", "item-id:0", "record-id",
              "category", "inventory", "item:1", "item-id:1", "record-id",
              "condition:1", "category", "inventory", "item:2",
              "item-id:2", "record-id", "condition:2"},
          "scan reacquires inventory, skips mismatches and stops on hit");
    Runtime miss;
    miss.items = {{0x44u, false}, {0x55u, true}};
    Check(!pal4::ScriptEquipmentInventoryQuery::HasUsableEquipment(
              -7, miss), "mismatch and true condition do not qualify");
    Check(miss.calls.back() == "condition:1" &&
              miss.calls.size() == 18u, "scan exhausts signed count");
}

}  // namespace

int main() {
    try {
        EarlyGates();
        Scan();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
