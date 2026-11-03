#include "pal4/mission_window_hover_lookup.h"

namespace pal4 {

std::uint32_t MissionWindowHoverLookup::Mission(
    const MissionWindowState& state,
    IMissionWindowMissionLookupRuntime& runtime) {
    const auto item = runtime.HoverItem(state.Read(0x54u));
    if (item == 0u) return 0u;
    return runtime.FindMission(runtime.ItemId(item, 0x138u));
}

std::uint32_t MissionWindowHoverLookup::Chapter(
    const MissionWindowState& state,
    IMissionWindowHoverLookupRuntime& runtime) {
    const auto item = runtime.HoverItem(state.Read(0x54u));
    if (item == 0u) return 0u;
    return runtime.FindChapter(runtime.ItemId(item, 0x138u));
}

}  // namespace pal4
