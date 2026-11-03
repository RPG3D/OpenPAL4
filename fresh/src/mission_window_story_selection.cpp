#include "pal4/mission_window_story_selection.h"

namespace pal4 {

void MissionWindowStorySelection::HandleClick(
    const MissionWindowState& state, std::uint32_t,
    IMissionWindowStorySelectionRuntime& runtime) {
    const auto story_list = state.Read(0x58u);
    const auto selected = runtime.FirstSelectedItem(story_list);
    if (selected == 0u) return;
    const auto selected_id = runtime.ItemId(selected, 0x138u);
    const auto mission_list = state.Read(0x54u);
    const auto rows = runtime.RowCount(mission_list);
    for (std::int32_t row = 0; row < rows; ++row) {
        const auto item = runtime.GridItem(
            mission_list, static_cast<std::uint32_t>(row), 0u);
        if (item == 0u) continue;
        const auto item_id = runtime.ItemId(item, 0x138u);
        if (item_id != selected_id) continue;
        const auto scroll_position = runtime.StoryScrollPosition(story_list);
        runtime.SetHoverRow(mission_list,
                            static_cast<std::uint32_t>(row));
        runtime.ScrollRowToVisible(mission_list,
                                   static_cast<std::uint32_t>(row));
        runtime.SetStoryScrollPosition(story_list, scroll_position);
        return;
    }
}

}  // namespace pal4
