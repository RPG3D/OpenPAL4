#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDataEquipmentManagerRuntime {
public:
    virtual ~IGameDataEquipmentManagerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t database, std::string_view name,
        std::uint32_t factory_address) = 0;
    [[nodiscard]] virtual std::uint32_t LookupTable(
        std::uint32_t database, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindProperty(
        std::uint32_t database, std::string_view name) = 0;
    virtual void LogError(std::string_view message) = 0;
};

class GameDataEquipmentManager final {
public:
    // IDA 0x4d5400: register Equip and EquipRelation object factories.
    [[nodiscard]] static std::uint8_t RegisterFactories(
        std::uint32_t object, IGameDataEquipmentManagerRuntime& runtime);
    // IDA 0x4d5430: verify four required tables/properties in binary order.
    [[nodiscard]] static std::uint8_t InitializeFromDatabase(
        std::uint32_t object, IGameDataEquipmentManagerRuntime& runtime);
};

}  // namespace pal4
