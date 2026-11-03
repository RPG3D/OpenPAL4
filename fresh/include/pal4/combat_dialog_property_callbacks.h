#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class ICombatDialogPropertyRuntime {
public:
    virtual ~ICombatDialogPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorFromFacet(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::int32_t ActorSlot(
        std::uint32_t actor) = 0;
    virtual void GetProperties(
        std::uint32_t actor,
        std::array<std::uint32_t, 6>& properties) = 0;
    [[nodiscard]] virtual std::uint8_t InitializeEffects(
        std::uint32_t actor, std::uint32_t slot) = 0;
};

class CombatDialogPropertyCallbacks final {
public:
    // IDA 0x57b120: considers the first three properties.
    [[nodiscard]] static std::uint8_t HandlePrimary(
        std::uint32_t event, ICombatDialogPropertyRuntime& runtime);
    // IDA 0x57b1a0: considers all six properties.
    [[nodiscard]] static std::uint8_t HandleExtended(
        std::uint32_t event, ICombatDialogPropertyRuntime& runtime);

private:
    [[nodiscard]] static std::uint8_t Handle(
        std::uint32_t event, std::uint32_t capacity,
        ICombatDialogPropertyRuntime& runtime);
};

}  // namespace pal4
