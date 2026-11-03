#include "pal4/save_window_list_events.h"

namespace pal4 {

std::uint8_t SaveWindowListEvents::OnPageButtonClick(
    SaveWindowState& state, std::uint32_t event,
    ISaveWindowListEventRuntime& runtime) {
    const auto button = runtime.ReadWord(event + 8u);
    const auto page = static_cast<std::int32_t>(
        runtime.ReadWord(button + 0x118u));
    runtime.PopulateSaveList(state, page, 0u);
    return 1u;
}

std::uint8_t SaveWindowListEvents::OnListClick(
    SaveWindowState& state, std::uint32_t,
    ISaveWindowListEventRuntime& runtime) {
    runtime.DisplayHoverItemInfo(state);
    return 1u;
}

}  // namespace pal4
