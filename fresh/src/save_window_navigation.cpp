#include "pal4/save_window_navigation.h"

namespace pal4 {

std::uint8_t SaveWindowNavigation::OnMemberEvent(
    SaveWindowState& state, std::uint32_t member_address,
    std::uint32_t event_address,
    ISaveWindowNavigationRuntime& runtime) {
    const auto code = runtime.ReadWord(event_address + 0x10u);
    if (code == 1u) {
        const auto base_event = runtime.ConstructBaseEvent();
        try {
            runtime.Close(state, member_address - 0x28u, base_event);
        } catch (...) {
            runtime.DestroyBaseEvent(base_event);
            throw;
        }
        runtime.DestroyBaseEvent(base_event);
        return 1u;
    }

    const auto list = state.Read(0x54u);
    if (runtime.RowCount(list) == 0u) return 0u;
    switch (code) {
    case 200u:
        if (runtime.HoverItem(list) != 0u &&
            static_cast<std::int32_t>(runtime.ReadWord(list + 0x4c0u)) > 0)
            runtime.UpHoverItem(list);
        return 1u;
    case 208u:
        if (runtime.HoverItem(list) != 0u &&
            runtime.ReadWord(list + 0x4c0u) < runtime.RowCount(list) - 1u)
            runtime.DownHoverItem(list);
        return 1u;
    case 203u: {
        const auto page = static_cast<std::int32_t>(
            runtime.ReadWord(state.Read(0x5cu) + 0x118u));
        if (page > 0) runtime.PopulateSaveList(state, page - 1, 0u);
        return 1u;
    }
    case 205u: {
        const auto page = static_cast<std::int32_t>(
            runtime.ReadWord(state.Read(0x5cu) + 0x118u));
        if (page < 9) runtime.PopulateSaveList(state, page + 1, 0u);
        return 1u;
    }
    case 28u:
        runtime.DisplayHoverItemInfo(state);
        return 1u;
    default:
        return 0u;
    }
}

}  // namespace pal4
