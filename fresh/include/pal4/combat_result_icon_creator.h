#pragma once

#include "pal4/combat_console_ui_initializer.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatResultIconRuntime : public ICombatConsoleUiRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void PlaySound(std::string_view name,
                           std::uint32_t mode) = 0;
};

class CombatResultIconCreator final {
public:
    static constexpr std::uint32_t kVtable = 0x843e84u;

    // IDA ui_createCombatResultIcon at 0x548520.
    [[nodiscard]] static std::uint8_t Create(
        std::uint32_t element,
        ICombatResultIconRuntime& runtime);
};

}  // namespace pal4
