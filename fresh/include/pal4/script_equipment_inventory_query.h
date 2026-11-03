#pragma once

#include <cstdint>

namespace pal4 {

class IScriptEquipmentInventoryQueryRuntime {
public:
    virtual ~IScriptEquipmentInventoryQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual bool HasRecord(std::uint32_t table,
                                          std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindRecord(
        std::uint32_t table, std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    [[nodiscard]] virtual std::int32_t ElementCount(
        std::uint32_t collection, std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t ElementAt(
        std::uint32_t collection, std::uint32_t category,
        std::uint32_t index) = 0;
    [[nodiscard]] virtual bool CheckPlayersCondition(
        std::uint32_t element) = 0;
};

class ScriptEquipmentInventoryQuery final {
public:
    // IDA 0x5d8180, giCheckEquipInInventory.
    [[nodiscard]] static bool HasUsableEquipment(
        std::int32_t id, IScriptEquipmentInventoryQueryRuntime& runtime);
};

}  // namespace pal4
