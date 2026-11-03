#pragma once

#include "pal4/inventory_item_slots.h"

#include <cstdint>

namespace pal4 {

class IScriptPlayerEquipmentRuntime : public IInventoryItemSlotsRuntime {
public:
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint8_t IsEquipment(
        std::uint32_t table, std::uint32_t equipment_id) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessInventoryCollection(
        std::uint32_t collection, std::uint32_t equipment_id) = 0;
    [[nodiscard]] virtual InventoryItemSlotsState ReadEquipmentSlots(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t RefreshEquipmentState(
        std::uint32_t player_state) = 0;
};

class ScriptPlayerEquipment final {
public:
    // IDA 0x5d5200, giAddPlayerEquip.
    [[nodiscard]] static std::uint8_t Add(
        std::int32_t player_id, std::uint32_t equipment_id,
        IScriptPlayerEquipmentRuntime& runtime);
};

}  // namespace pal4
