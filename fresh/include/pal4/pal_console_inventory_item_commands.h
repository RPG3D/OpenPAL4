#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsoleInventoryItemRuntime {
public:
    virtual ~IPalConsoleInventoryItemRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual bool HasProperty(std::uint32_t database,
                                           std::int32_t item_id) = 0;
    [[nodiscard]] virtual bool HasEquipment(std::uint32_t equipment_table,
                                            std::int32_t item_id) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessProperty(
        std::uint32_t inventory, std::int32_t item_id,
        std::int32_t quantity) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessEquipment(
        std::uint32_t collection, std::int32_t item_id) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::uint32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsoleInventoryItemCommands final {
public:
    // IDA 0x5bbfe0, command "addProp".
    [[nodiscard]] static std::uint32_t AddProperty(
        const char* command, IPalConsoleInventoryItemRuntime& runtime);
    // IDA 0x5bc0b0, command "addEquip".
    [[nodiscard]] static std::uint32_t AddEquipment(
        const char* command, IPalConsoleInventoryItemRuntime& runtime);
};

}  // namespace pal4
