#pragma once
#include "pal4/binary_xml_config.h"
#include "pal4/game_arena_load_handler.h"

namespace pal4 {
// Consume the save page's temporary config through the existing XML/state
// runtime. Entry only: archive restoration happens on subsequent Loading ticks.
void EnterNativeArchiveLoading(const BinaryXmlNode& config,
                               IGameArenaLoadRuntime& runtime);
} // namespace pal4
