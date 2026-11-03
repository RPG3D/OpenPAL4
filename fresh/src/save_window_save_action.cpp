#include "pal4/save_window_save_action.h"

namespace pal4 {

std::uint8_t SaveWindowSaveAction::Confirm(
    SaveWindowState& state, std::int32_t response,
    ISaveWindowSaveActionRuntime& runtime) {
    if (response != 1) return 1u;
    const auto list = state.Read(0x54u);
    const auto item = runtime.HoverItem(list);
    if (item == 0u) return 1u;
    const auto slot = runtime.ReadSlot(item);
    runtime.SetGlobalSaveIndex(slot);
    runtime.SetGlobalScrollPosition(runtime.ScrollPosition(list));
    runtime.SaveSettings();
    runtime.SaveGameSlot(slot);
    runtime.PopulateSaveList(state, slot / 10, 1u);
    return 1u;
}

}  // namespace pal4
