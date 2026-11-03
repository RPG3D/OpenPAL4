#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDataEquipmentBindingRefreshRuntime {
public:
    virtual ~IGameDataEquipmentBindingRefreshRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BeginLog(
        std::string_view prefix, std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeRelationStructures(std::uint32_t object) = 0;
    virtual void InitializeEquipmentStructures(std::uint32_t object) = 0;
    virtual void BindEffects(std::uint32_t object) = 0;
    virtual void EndLog(std::uint32_t log, std::string_view prefix,
                        std::uint32_t table_name) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseLog(std::uint32_t log) = 0;
};

class GameDataEquipmentBindingRefresh final {
public:
    // IDA 0x4d7550: refresh EquipRelation bindings when counts diverge.
    [[nodiscard]] static std::uint32_t RefreshRelation(
        std::uint32_t table,
        IGameDataEquipmentBindingRefreshRuntime& runtime);
    // IDA 0x4d7780: refresh Equip bindings when counts diverge.
    [[nodiscard]] static std::uint32_t RefreshEquipment(
        std::uint32_t table,
        IGameDataEquipmentBindingRefreshRuntime& runtime);
};

}  // namespace pal4
