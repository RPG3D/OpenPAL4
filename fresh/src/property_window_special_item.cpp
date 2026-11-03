#include "pal4/property_window_special_item.h"

namespace pal4 {

std::uint8_t PropertyWindowSpecialItem::UseMagicItem(
    const std::uint32_t frame, const std::uint32_t context,
    IPropertyWindowSpecialItemRuntime& runtime) {
    runtime.ToggleModel(frame, context, 0u);
    runtime.UpdateQuestModel(frame, 0u);
    runtime.RemoveItem(runtime.Inventory(), 3182u, 1u);
    return runtime.ProcessEvents(runtime.UIFrameManager() + 0x104u,
                                 kEventId, {1u, 0u});
}

}  // namespace pal4
