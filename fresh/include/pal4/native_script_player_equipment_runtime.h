#pragma once

#include "pal4/script_player_equipment.h"

namespace pal4 {

class NativeBaseObjectMemory;
class NativeInventorySession;
class NativePlayerManagerSession;
class NativePlayerScriptRefreshRuntime;

class NativeScriptPlayerEquipmentRuntime final :
    public IScriptPlayerEquipmentRuntime {
public:
    NativeScriptPlayerEquipmentRuntime(
        NativePlayerManagerSession& players,
        NativeInventorySession& inventory,
        NativeBaseObjectMemory& player_memory,
        NativePlayerScriptRefreshRuntime& refresh)
        : players_(players), inventory_(inventory),
          player_memory_(player_memory), refresh_(refresh) {}

    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override;
    std::uint8_t IsEquipment(std::uint32_t table,
                             std::uint32_t equipment_id) override;
    std::uint32_t ProcessInventoryCollection(
        std::uint32_t collection, std::uint32_t equipment_id) override;
    InventoryItemSlotsState ReadEquipmentSlots(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t RefreshEquipmentState(std::uint32_t player_state) override;

    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t GetGameDatabase() override;
    std::uint32_t ProcessGameDatabaseArray(std::uint32_t array,
                                           std::uint32_t value) override;
    std::uint32_t GetInventory() override;
    std::uint32_t FindInventoryEntry(std::uint32_t slots,
                                     std::uint32_t id,
                                     std::uint32_t category) override;
    std::uint32_t ProcessEntry(std::uint32_t first, std::uint32_t second,
                               std::uint32_t entry) override;
    void InsertEntry(std::uint32_t destination,
                     std::uint32_t entry) override;

private:
    NativePlayerManagerSession& players_;
    NativeInventorySession& inventory_;
    NativeBaseObjectMemory& player_memory_;
    NativePlayerScriptRefreshRuntime& refresh_;
};

}  // namespace pal4
