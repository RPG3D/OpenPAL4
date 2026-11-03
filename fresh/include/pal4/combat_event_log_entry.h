#pragma once
#include "pal4/combat_event_logger.h"
#include <optional>
#include <string>
#include <string_view>
namespace pal4 {
// Game log text policy from57E1E0, shared by the legacy game boundary and
// modern native stream owner. No private CRT stream layout is involved.
[[nodiscard]] std::string FormatCombatLogEntry(const CombatLogTime&,std::uint32_t level,
    std::string_view message,std::optional<std::string_view> detail=std::nullopt);
}
