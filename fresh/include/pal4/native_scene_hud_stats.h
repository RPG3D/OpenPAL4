#pragma once

#include "pal4/minimap_scene_ui_stats.h"
#include <windows.h>
#include <array>

namespace pal4 {

// Original CEGUI objects are borrowed for the duration of this update.
void UpdateNativeSceneHudStats(HMODULE module,
    const std::array<std::uint32_t, 3>& windows,
    IMinimapSceneUiStatsDataRuntime& players, bool& flash);

}  // namespace pal4
