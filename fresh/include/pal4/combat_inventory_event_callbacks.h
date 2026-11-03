#pragma once

#include <cstdint>

namespace pal4 {

class ICombatInventoryEventRuntime {
public:
    virtual ~ICombatInventoryEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryInstance() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateAndClampCounter(
        std::uint32_t inventory, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    virtual void FindProperty(std::uint32_t database,
                              std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessItem(
        std::uint32_t inventory, std::uint32_t item,
        std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    virtual void CreateMonster(std::uint32_t manager,
                               std::uint32_t monster,
                               std::uint32_t mode,
                               std::uint32_t flags) = 0;
};

class CombatInventoryEventCallbacks final {
public:
    // IDA 0x57b0a0.
    [[nodiscard]] static std::uint32_t UpdateCounter(
        std::uint32_t event, ICombatInventoryEventRuntime& runtime);
    // IDA 0x57b0c0.
    [[nodiscard]] static std::uint32_t ProcessItem(
        std::uint32_t event, ICombatInventoryEventRuntime& runtime);
    // IDA 0x57b0f0.
    [[nodiscard]] static std::int32_t CreateMonsters(
        std::uint32_t event, ICombatInventoryEventRuntime& runtime);
};

}  // namespace pal4
