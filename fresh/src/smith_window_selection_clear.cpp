#include "pal4/smith_window.h"

#include <cstdint>

namespace pal4 {

void SmithWindow::ClearEquipmentSelection(
    const std::uint32_t object,
    ISmithWindowSelectionClearRuntime& runtime) {
    const auto list = runtime.ReadWord(object + 0x40u);
    runtime.SetSortDirection(list, 0u);
    runtime.ResetList(list);
    if (!runtime.ColumnsInitialized()) {
        for (auto count = runtime.ColumnCount(list); count > 0; --count)
            runtime.RemoveColumn(list, 0u);
        runtime.SetUserColumnSizing(list, false);
        runtime.SetUserColumnDragging(list, false);
        runtime.SetUserSortControl(list, false);
        runtime.AddColumn(list, 0u, 0.5f);
        runtime.SetColumnLabelImage(list, 0u, "zi0", "mingcheng");
        runtime.AddColumn(list, 1u, 0.395f);
        runtime.SetColumnLabelImage(list, 1u, "zi0", "dengji");
        runtime.SetColumnsInitialized(true);
    }
    UpdateEquipmentItemName(object, 0u, runtime);
    UpdateEquipmentItemDescription(object, 0u, runtime);
    runtime.WriteByte(object + 0xacu, 0u);
    runtime.SetText(runtime.ReadWord(object + 0x6cu), {});
}

}  // namespace pal4
