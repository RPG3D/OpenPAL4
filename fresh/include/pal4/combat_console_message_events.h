#pragma once

#include "pal4/combat_console_hint_events.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatConsoleMessageRuntime : public ICombatConsoleHintRuntime {
public:
    [[nodiscard]] virtual std::uint32_t FindCombatRecord(
        std::uint32_t database, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::string FormatText(
        std::uint32_t format_text,
        std::span<const std::uint32_t> arguments) = 0;
    virtual void ShowFormattedHint(
        std::uint32_t window, std::string_view text,
        const std::array<std::uint32_t, 2>& position,
        std::uint32_t duration_bits) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    virtual void PlaySoundEffect(std::uint32_t event,
                                 std::string_view sound_name,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupFormattedText(
        std::string_view text) = 0;
};

class CombatConsoleMessageEvents final {
public:
    // IDA 0x5409a0 and 0x540b40.
    [[nodiscard]] static std::uint32_t ShowCombatRecord(
        std::uint32_t event, ICombatConsoleMessageRuntime& runtime);
    [[nodiscard]] static std::uint32_t ShowSignedValue(
        std::uint32_t event, ICombatConsoleMessageRuntime& runtime);
};

}  // namespace pal4
