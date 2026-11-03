#include "pal4/mission_window_quest_list.h"

#include <new>

namespace pal4 {

void MissionWindowQuestList::Populate(
    MissionWindowState& state, IMissionWindowQuestListRuntime& runtime) {
    runtime.SetImage(state.Read(0x80u), "zi0", "juqingzhangjie");
    runtime.SetImage(state.Read(0x7cu), "zi0", "juqinggaishu");
    state.bytes[0x88u] = 0u;
    runtime.SetSelected(state.Read(0x64u), true);
    runtime.SetSelected(state.Read(0x60u), false);

    const auto quest_list = state.Read(0x54u);
    const auto story_list = state.Read(0x58u);
    while (runtime.ColumnCount(quest_list) != 0u) {
        runtime.RemoveColumn(quest_list, 0u);
    }
    runtime.ResetList(quest_list);
    const auto title = runtime.TriggerTitle(26u);
    const auto utf8_title = runtime.ConvertToUtf8(title);
    runtime.AddColumn(quest_list, utf8_title, 0u, 0.89999998f);
    runtime.ResetList(story_list);
    const auto width = runtime.AbsoluteColumnWidth(story_list, 0u);

    auto records = runtime.LoadQuestRecords();
    try {
        std::vector<const MissionQuestRecord*> visible_records;
        for (const auto& record : records) {
            if (IsVisible(record.id, runtime.StoryVisibilityLimit())) {
                visible_records.push_back(&record);
            }
        }

        for (const auto* selected : visible_records) {
            const auto& record = *selected;
            runtime.AddRow(quest_list, 0u);
            const auto utf8_record_title = runtime.ConvertToUtf8(record.title);
            const auto plain_item = runtime.CreatePlainItem(
                quest_list, utf8_record_title, kPlainItemBytes,
                kPlainItemVtable);
            if (plain_item == 0u) throw std::bad_alloc{};
            runtime.WriteItemId(plain_item,
                                static_cast<std::uint32_t>(record.id));
            runtime.SetItem(quest_list, plain_item, 0u,
                            runtime.RowCount(quest_list) - 1u);

            runtime.AddRow(story_list, 0u);
            const auto utf8_description = runtime.ConvertToUtf8(
                record.description);
            const auto rich_item = runtime.CreateRichItem(
                utf8_description, width, kRichItemBytes);
            if (rich_item == 0u) throw std::bad_alloc{};
            runtime.WriteItemId(rich_item,
                                static_cast<std::uint32_t>(record.id));
            runtime.SetItem(story_list, rich_item, 0u,
                            runtime.RowCount(story_list) - 1u);
        }
        if (runtime.RowCount(quest_list) != 0u) {
            const auto last = runtime.RowCount(quest_list) - 1u;
            runtime.SetHoverRow(quest_list, last);
            runtime.ScrollRowToVisible(quest_list, last);
        }
        runtime.SetMissionPanelVisibility(state, false);
    } catch (...) {
        runtime.ReleaseQuestRecords();
        throw;
    }
    runtime.ReleaseQuestRecords();
}

}  // namespace pal4
