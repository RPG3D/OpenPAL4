#pragma once

#include "pal4/player_manager_slot.h"

namespace pal4 {

// IDA 842F60 / 8430E8 have identical property and skill dispatch entries.
// Only destructor+F8 and refresh notification+114 differ. The existing native
// skill callbacks preserve the snapshot's empty refresh notification.
[[nodiscard]] inline bool IsNativePlayerScriptFacet(std::uint32_t table) noexcept {
    return table == PlayerManagerSlot::kScriptVtable ||
           table == PlayerManagerSlot::kScriptCleanupVtable;
}

}  // namespace pal4
