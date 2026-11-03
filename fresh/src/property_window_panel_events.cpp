#include "pal4/property_window_panel_events.h"

namespace pal4 {

bool PropertyWindowPanelEvents::HandleCategoryClick(
    PropertyWindowState& state, std::uint32_t event,
    IPropertyWindowPanelEventsRuntime& runtime) {
    const auto code = static_cast<std::int32_t>(
        runtime.ReadWord(event + 0x10u));
    if (code != 2 && (code < 3 || code >= 8)) return false;
    state.Write(0x6cu, code == 2 ? 5u
                                 : static_cast<std::uint32_t>(code - 3));
    runtime.UpdateCategoryPanel(state);
    return true;
}

std::uint32_t PropertyWindowPanelEvents::ConstructCallback(
    std::uint32_t object, IPropertyWindowPanelEventsRuntime& runtime) {
    runtime.WriteWord(object, kCallbackVtable);
    return object;
}

bool PropertyWindowPanelEvents::HandleItemPanelEvent(
    std::uint32_t callback, std::uint32_t event,
    IPropertyWindowPanelEventsRuntime& runtime) {
    const auto code = runtime.ReadWord(event + 0x10u);
    if (code == 1u) {
        runtime.RemoveActiveMember(callback);
        runtime.HideItemPanel(runtime.ReadWord(callback + 4u));
        return true;
    }

    const auto owner = runtime.ReadWord(callback + 4u);
    const auto list = runtime.ReadWord(owner + 0x58u);
    if (runtime.RowCount(list) == 0u) return false;
    if (code == 200u) {
        if (runtime.HoverItem(list) != 0u) runtime.MoveHoverUp(list);
        return true;
    }
    if (code == 208u) {
        if (runtime.HoverItem(list) != 0u) runtime.MoveHoverDown(list);
        return true;
    }
    if (code == 28u) {
        runtime.ShowItemSelection(owner, {400.0f, 100.0f});
        return true;
    }
    return false;
}

}  // namespace pal4
