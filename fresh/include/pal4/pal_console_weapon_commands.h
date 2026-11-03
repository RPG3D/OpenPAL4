#pragma once

#include <cstdint>

namespace pal4 {

class IPalConsoleWeaponRuntime {
public:
    virtual ~IPalConsoleWeaponRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ApplyEquipmentVisuals(
        std::uint32_t player) = 0;
    [[nodiscard]] virtual std::int32_t ClearWeaponEffects(
        std::uint32_t player) = 0;
};

class PalConsoleWeaponCommands final {
public:
    // IDA 0x5bd3e0 / 0x5bd420.
    [[nodiscard]] static std::int32_t TakeOn(
        IPalConsoleWeaponRuntime& runtime);
    [[nodiscard]] static std::int32_t TakeOff(
        IPalConsoleWeaponRuntime& runtime);
};

}  // namespace pal4
