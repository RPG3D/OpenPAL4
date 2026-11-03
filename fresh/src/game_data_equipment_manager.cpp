#include "pal4/game_data_equipment_manager.h"

#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

void Require(std::uint32_t found, std::string_view name,
             IGameDataEquipmentManagerRuntime& runtime) {
    if (found) return;
    std::string message =
        "ERROR [ palDBEquipmentManager::verifyDataTable < the table ";
    message += name;
    message += "  doesn't exist>]";
    runtime.LogError(message);
    throw std::runtime_error(message);
}

}  // namespace

std::uint8_t GameDataEquipmentManager::RegisterFactories(
    std::uint32_t object, IGameDataEquipmentManagerRuntime& runtime) {
    const auto database = runtime.ReadWord(object);
    (void)runtime.RegisterFactory(database, "Equip", 0x4d6040);
    return static_cast<std::uint8_t>(runtime.RegisterFactory(
        database, "EquipRelation", 0x4d5640));
}

std::uint8_t GameDataEquipmentManager::InitializeFromDatabase(
    std::uint32_t object, IGameDataEquipmentManagerRuntime& runtime) {
    const auto database = runtime.ReadWord(object);
    const auto equipment = runtime.LookupTable(database, "Equip");
    runtime.WriteWord(object + 8, equipment);
    Require(equipment, "Equip", runtime);

    const auto relation = runtime.LookupTable(database, "EquipRelation");
    runtime.WriteWord(object + 4, relation);
    Require(relation, "EquipRelation", runtime);

    const auto equipment_class = runtime.FindProperty(database, "EquipClass");
    runtime.WriteWord(object + 16, equipment_class);
    Require(equipment_class, "EquipClass", runtime);

    const auto equipment_type = runtime.FindProperty(database, "EquipType");
    runtime.WriteWord(object + 12, equipment_type);
    Require(equipment_type, "EquipType", runtime);
    return 1;
}

}  // namespace pal4
