#include "pal4/mission_window_list.h"

#include <new>

namespace pal4 {

void MissionWindowList::Populate(MissionWindowState& state,
                                 IMissionWindowListRuntime& runtime) {
    runtime.SetImage(state.Read(0x80u), "zi0", "renwutishi");
    runtime.SetImage(state.Read(0x7cu), "zi0", "renwutishia");
    state.bytes[0x88u] = 1u;
    runtime.SetSelected(state.Read(0x60u), true);
    runtime.SetSelected(state.Read(0x64u), false);

    const auto list = state.Read(0x54u);
    while (runtime.ColumnCount(list) != 0u) {
        runtime.RemoveColumn(list, 0u);
    }
    runtime.ResetList(list);
    const auto heading = runtime.TriggerTitle(25u);
    const auto utf8_heading = runtime.ConvertToUtf8(heading);
    runtime.AddColumn(list, utf8_heading, 0u, 0.89999998f);

    auto records = runtime.LoadMissionRecords();
    try {
        for (const auto& slot : records) {
            if (!slot) continue;
            runtime.AddRow(list, 0u);
            const auto utf8_title = runtime.ConvertToUtf8(slot->title);
            const auto item = runtime.CreateItem(
                list, utf8_title, kItemBytes, kItemVtable);
            if (item == 0u) throw std::bad_alloc{};
            runtime.WriteItemId(item, slot->id);
            runtime.WriteItemEnabled(item, slot->availability > 0);
            runtime.SetItem(list, item, 0u, runtime.RowCount(list) - 1u);
        }
        if (runtime.RowCount(list) != 0u) {
            runtime.SetHoverRow(list, 0);
        }
        runtime.SetMissionPanelVisibility(state, true);
    } catch (...) {
        runtime.ReleaseMissionRecords();
        throw;
    }
    runtime.ReleaseMissionRecords();
}

}  // namespace pal4
