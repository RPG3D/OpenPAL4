#include "pal4/smith_window.h"

namespace pal4 {

void SmithWindow::RefreshAfterInventoryChange(
    const std::uint32_t object,
    ISmithWindowInventoryRefreshRuntime& runtime) {
    const auto id = runtime.PrescriptionHoverItemId(object);
    runtime.TouchSmithRecord(id);

    const auto begin = runtime.ReadWord(object + 0xbcu);
    const auto end = runtime.ReadWord(object + 0xc0u);
    for (auto cursor = begin;
         cursor != end; cursor += 4u) {
        if (runtime.HasInventoryItem(runtime.ReadWord(cursor), 7u))
            continue;
        for (auto next = cursor + 4u; next != end; next += 4u)
            runtime.WriteWord(next - 4u, runtime.ReadWord(next));
        runtime.WriteWord(object + 0xc0u, end - 4u);
        break;
    }

    runtime.CollectMatchingInventoryItemIds(object, id);
    runtime.RefreshCollectedEquipmentList(object);
}

}  // namespace pal4
