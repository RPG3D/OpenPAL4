#include "pal4/mission_window_details_panel.h"

namespace pal4 {

void MissionWindowDetailsPanel::Update(
    const MissionWindowState& state,
    IMissionWindowDetailsPanelRuntime& runtime) {
    if (state.bytes[0x88u] != 0u) {
        const auto mission = MissionWindowHoverLookup::Mission(state, runtime);
        MissionWindowQuestDetails::Update(state, mission, runtime);
        return;
    }

    const auto chapter = MissionWindowHoverLookup::Chapter(state, runtime);
    if (chapter == 0u) return;
    const auto story_list = state.Read(0x58u);
    const auto rows = runtime.RowCount(story_list);
    if (rows <= 0) return;
    for (std::int32_t row = 0; row < rows; ++row) {
        const auto item = runtime.GridItem(
            story_list, static_cast<std::uint32_t>(row), 0u);
        if (item == 0u) continue;
        const auto record_id = runtime.RecordId(chapter);
        const auto item_id = runtime.ItemId(item, 0x138u);
        if (record_id == item_id) {
            runtime.SetHoverRow(story_list,
                                static_cast<std::uint32_t>(row));
            runtime.ScrollRowToTop(story_list,
                                   static_cast<std::uint32_t>(row));
            return;
        }
    }
}

}  // namespace pal4
