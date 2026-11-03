#pragma once

#include "pal4/combat_console_image_events.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatConsoleNumberRuntime : public ICombatConsoleVisualRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ActorAttribute(
        std::uint32_t actor, std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupFormattedText(
        std::string_view text) = 0;
};

class CombatConsoleNumberEvents final {
public:
    static constexpr std::uint32_t kElementSize = 92u;
    static constexpr std::uint32_t kVtable = 0x842698u;

    // IDA 0x5404b0.
    [[nodiscard]] static std::uint32_t Display(
        std::uint32_t event, ICombatConsoleNumberRuntime& runtime);
};

}  // namespace pal4
