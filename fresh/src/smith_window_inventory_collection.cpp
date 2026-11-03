#include "pal4/smith_window.h"

namespace pal4 {

std::uint8_t SmithWindow::CollectMatchingInventoryItemIds(
    const std::uint32_t object, const std::uint32_t input,
    ISmithWindowInventoryCollectionRuntime& runtime) {
    SmithWindowWordArray::Clear(object + 0xb8u, runtime);

    const auto id = runtime.ResolveSmithItemId(input);
    if (id == -1) return 0xffu;
    const auto unsigned_id = static_cast<std::uint32_t>(id);
    if (!runtime.HasDatabaseItem(unsigned_id)) return 0u;

    const auto item = runtime.DatabaseItem(unsigned_id);
    const auto category = runtime.ReadWord(item + 0x94u);
    const auto count = runtime.InventorySize(category);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto inventory_item = runtime.InventoryAt(
            category, static_cast<std::uint32_t>(index));
        if (runtime.ReadWord(inventory_item + 0x90u) == unsigned_id)
            SmithWindowWordArray::Append(
                object + 0xb8u,
                runtime.ReadWord(inventory_item + 0x190u), runtime);
    }
    return static_cast<std::uint8_t>(count);
}

}  // namespace pal4
