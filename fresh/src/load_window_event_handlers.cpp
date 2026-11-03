#include "pal4/load_window_event_handlers.h"

namespace pal4 {

std::uint8_t LoadWindowEventHandlers::OnPageButton(
    LoadWindowState& state, std::uint32_t event_address,
    ILoadWindowEventRuntime& runtime) {
    const auto window = runtime.EventWindow(event_address + 8u);
    auto& save_list = runtime.SaveListRuntime();
    const auto page = static_cast<std::int32_t>(
        save_list.WindowUserId(window));
    LoadWindowSaveList::Populate(state, page, 0u, save_list);
    return 1u;
}

std::uint8_t LoadWindowEventHandlers::OnMouseClick(
    LoadWindowState& state, ILoadWindowEventRuntime& runtime) {
    return OnItemClick(state, runtime);
}

std::uint8_t LoadWindowEventHandlers::OnItemClick(
    LoadWindowState& state, ILoadWindowEventRuntime& runtime) {
    const auto item = runtime.HoverItem(state.Read(0x54u));
    if (item == 0u || runtime.ItemWord(item, 0x1ecu) == 0u) return 1u;

    auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) runtime.InitializeSaveLoadCallback(callback, state);
    try {
        auto text = runtime.TriggerText(0xe5u);
        if (text == 0u) text = runtime.EmptyText();
        const auto dialog = runtime.CreateCommonDialog();
        runtime.ShowDialog(dialog, text, 0.3f, 0.4f, 3u, callback);
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
    return 1u;
}

std::uint8_t LoadWindowEventHandlers::OnHoverRowChanged(
    LoadWindowState& state, ILoadWindowEventRuntime& runtime) {
    (void)runtime.HoverItem(state.Read(0x54u));
    return 1u;
}

std::uint8_t LoadWindowEventHandlers::HandleListEvent(
    LoadWindowState& state, std::uint32_t event_address,
    ILoadWindowEventRuntime& runtime) {
    const auto code = runtime.EventCode(event_address + 0x10u);
    if (code == 1u) {
        runtime.DispatchBaseEvent(state);
        return 1u;
    }

    const auto list = state.Read(0x54u);
    auto& save_list = runtime.SaveListRuntime();
    if (save_list.RowCount(list) == 0u) return 0u;

    switch (code) {
    case 200u:
        if (runtime.HoverItem(list) != 0u &&
            runtime.HoverRowIndex(list) > 0) runtime.UpHoverItem(list);
        return 1u;
    case 208u:
        if (runtime.HoverItem(list) != 0u) {
            const auto row = static_cast<std::uint32_t>(
                runtime.HoverRowIndex(list));
            if (row < save_list.RowCount(list) - 1u)
                runtime.DownHoverItem(list);
        }
        return 1u;
    case 203u: {
        const auto page = static_cast<std::int32_t>(
            save_list.WindowUserId(state.Read(0x5cu)));
        if (page > 0) LoadWindowSaveList::Populate(
            state, page - 1, 0u, save_list);
        return 1u;
    }
    case 205u: {
        const auto page = static_cast<std::int32_t>(
            save_list.WindowUserId(state.Read(0x5cu)));
        if (page < 9) LoadWindowSaveList::Populate(
            state, page + 1, 0u, save_list);
        return 1u;
    }
    case 28u:
        return OnItemClick(state, runtime);
    default:
        return 0u;
    }
}

}  // namespace pal4
