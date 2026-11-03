#include "pal4/pal_console_inventory_item_commands.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalConsoleInventoryItemRuntime {
    bool property_found = true;
    bool equipment_found = true;
    std::vector<std::string> calls;
    std::string output;
    std::int32_t id_seen = 0;
    std::int32_t quantity_seen = 0;

    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x1000u;
    }
    bool HasProperty(std::uint32_t database,
                     std::int32_t item_id) override {
        Check(database == 0x1000u, "property database base");
        calls.emplace_back("property-lookup");
        id_seen = item_id;
        return property_found;
    }
    bool HasEquipment(std::uint32_t equipment_table,
                      std::int32_t item_id) override {
        Check(equipment_table == 0x1058u, "equipment table offset");
        calls.emplace_back("equipment-lookup");
        id_seen = item_id;
        return equipment_found;
    }
    std::uint32_t Inventory() override {
        calls.emplace_back("inventory");
        return 0x2000u;
    }
    std::uint32_t ProcessProperty(std::uint32_t inventory,
                                  std::int32_t item_id,
                                  std::int32_t quantity) override {
        Check(inventory == 0x2000u && item_id == id_seen,
              "property processing operands");
        calls.emplace_back("process-property");
        quantity_seen = quantity;
        return 0x3456u;
    }
    std::uint32_t ProcessEquipment(std::uint32_t collection,
                                   std::int32_t item_id) override {
        Check(collection == 0x207cu && item_id == id_seen,
              "equipment collection operands");
        calls.emplace_back("process-equipment");
        return 0x4567u;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x3000u;
    }
    std::uint32_t AddOutputLine(std::uint32_t console,
                                std::string_view format) override {
        Check(console == 0x3000u, "error console");
        calls.emplace_back("output");
        output = format;
        return 0x5678u;
    }
};

void AddPropertySuccess() {
    Runtime runtime;
    Check(pal4::PalConsoleInventoryItemCommands::AddProperty(
              "17 -3 ignored", runtime) == 0x3456u &&
              runtime.id_seen == 17 && runtime.quantity_seen == -3 &&
              runtime.calls == std::vector<std::string>{
                  "database", "property-lookup", "inventory",
                  "process-property"},
          "property ID and signed quantity forwarded in native order");

    Runtime nonnumeric;
    Check(pal4::PalConsoleInventoryItemCommands::AddProperty(
              "bad none", nonnumeric) == 0x3456u &&
              nonnumeric.id_seen == 0 && nonnumeric.quantity_seen == 0,
          "atoi zero is accepted when database contains ID zero");
}

void AddEquipmentSuccess() {
    Runtime runtime;
    Check(pal4::PalConsoleInventoryItemCommands::AddEquipment(
              "-12 trailing", runtime) == 0x4567u &&
              runtime.id_seen == -12 &&
              runtime.calls == std::vector<std::string>{
                  "database", "equipment-lookup", "inventory",
                  "process-equipment"},
          "equipment lookup and collection offsets");
}

void MissingParameters() {
    for (const char* input :
         std::vector<const char*>{nullptr, "17"}) {
        Runtime runtime;
        Check(pal4::PalConsoleInventoryItemCommands::AddProperty(
                  input, runtime) == 0x5678u &&
                  runtime.calls == std::vector<std::string>{
                      "console", "output"} &&
                  runtime.output ==
                      "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
              "addProp requires two fields");
    }
    Runtime equipment;
    Check(pal4::PalConsoleInventoryItemCommands::AddEquipment(
              nullptr, equipment) == 0x5678u &&
              equipment.calls == std::vector<std::string>{
                  "console", "output"} &&
              equipment.output ==
                  "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
          "addEquip requires one field");
}

void InvalidItemIds() {
    Runtime property;
    property.property_found = false;
    Check(pal4::PalConsoleInventoryItemCommands::AddProperty(
              "17 2", property) == 0x5678u &&
              property.calls == std::vector<std::string>{
                  "database", "property-lookup", "console", "output"} &&
              property.output ==
                  "\xb5\xc0\xbe\xdf\x49\x44\xb4\xed\xce\xf3",
          "unknown property does not initialize inventory");

    Runtime equipment;
    equipment.equipment_found = false;
    Check(pal4::PalConsoleInventoryItemCommands::AddEquipment(
              "17", equipment) == 0x5678u &&
              equipment.calls == std::vector<std::string>{
                  "database", "equipment-lookup", "console", "output"} &&
              equipment.output ==
                  "\xb5\xc0\xbe\xdf\x49\x44\xb4\xed\xce\xf3",
          "unknown equipment shares item-ID error");
}

}  // namespace

int main() {
    try {
        AddPropertySuccess();
        AddEquipmentSuccess();
        MissingParameters();
        InvalidItemIds();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
