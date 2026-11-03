#pragma once
#include "pal4/portable/original_database.h"
#include "pal4/portable/player_equipment.h"

namespace pal4::portable {
struct OriginalPlayerData {
    RoleCatalog roles;
    std::vector<UpgradeRecord> upgrades;
    EquipmentCatalog equipment;
};
OriginalPlayerData ReadOriginalPlayerData(const OriginalDatabaseCatalogs& database);
}
