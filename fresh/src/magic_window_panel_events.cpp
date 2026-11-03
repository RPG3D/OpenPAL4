#include "pal4/magic_window_panel_events.h"

namespace pal4 {

bool MagicWindowPanelEvents::HandlePanelClick(
    std::int32_t event_code, const MagicWindowPanelState& panel,
    IMagicWindowPanelEventsRuntime& runtime) {
    if (event_code < 2 || event_code >= 8)
        return false;
    // IDA45A640 receives the navigation subobject at page+0x28. Convert
    // this-relative +0x78 to the full-page panel offsets before indexing.
    runtime.HandleMouseEvent(panel.At(0xa0u +
                                      4u * static_cast<std::uint32_t>(event_code)));
    return true;
}

bool MagicWindowPanelEvents::HandleEvent(
    std::uint32_t member, std::int32_t event_code,
    MagicWindowPanelEventState& state, const MagicWindowPanelState& panel,
    IMagicWindowPanelEventsRuntime& runtime) {
    switch (event_code) {
    case 1:
        runtime.RemoveActive(member);
        runtime.SetStuntPanelVisible(state.panel_owner, false);
        return true;
    case 200: {
        if (state.highlight_active) return true;
        const auto row = MagicWindowSelection::SelectedMagicRow(panel, runtime);
        if (row == 0) SetHighlight(true, state, panel, runtime);
        else if (row > 0) runtime.UpdateMagicDetails(state.panel_owner, row - 1);
        return true;
    }
    case 208: {
        if (state.highlight_active) {
            runtime.UpdateMagicDetails(state.panel_owner, 0);
            SetHighlight(false, state, panel, runtime);
            return true;
        }
        const auto row = MagicWindowSelection::SelectedMagicRow(panel, runtime);
        if (row >= 0 && row < 5)
            runtime.UpdateMagicDetails(state.panel_owner, row + 1);
        return true;
    }
    case 203:
        if (state.highlight_active) {
            const auto secondary = panel.At(0x84u);
            runtime.SetHoverFlag(secondary, false);
            runtime.RequestRedraw(secondary);
            runtime.HandleMouseEvent(panel.At(0x80u));
        }
        return true;
    case 205:
        if (state.highlight_active) {
            const auto primary = panel.At(0x80u);
            runtime.SetHoverFlag(primary, false);
            runtime.RequestRedraw(primary);
            runtime.HandleMouseEvent(panel.At(0x84u));
        }
        return true;
    case 28: {
        const auto row = MagicWindowSelection::SelectedMagicRow(panel, runtime);
        if (row >= 0 && row < 6)
            runtime.HandleMouseEvent(panel.At(0xf0u + 4u *
                                                    static_cast<std::uint32_t>(row)));
        return true;
    }
    default:
        return false;
    }
}

bool MagicWindowPanelEvents::HandleStuntRowsEvent(
    std::uint32_t member, std::int32_t event_code,
    MagicWindowPanelEventState& state, const MagicWindowPanelState& panel,
    IMagicWindowPanelEventsRuntime& runtime) {
    switch (event_code) {
    case 200: {
        if (state.highlight_active) return true;
        const auto row = MagicWindowSelection::SelectedStuntRow(panel, runtime);
        if (row == 0) SetHighlight(true, state, panel, runtime);
        else if (row > 0) runtime.UpdateStuntDetails(state.panel_owner, row - 1);
        return true;
    }
    case 208: {
        if (state.highlight_active) {
            runtime.UpdateStuntDetails(state.panel_owner, 0);
            SetHighlight(false, state, panel, runtime);
            return true;
        }
        const auto row = MagicWindowSelection::SelectedStuntRow(panel, runtime);
        if (row >= 0 && row < 6)
            runtime.UpdateStuntDetails(state.panel_owner, row + 1);
        return true;
    }
    case 28: {
        const auto row = MagicWindowSelection::SelectedStuntRow(panel, runtime);
        if (row < 0 || row >= 7) return true;
        const auto target = panel.At(0x108u + 4u *
                                             static_cast<std::uint32_t>(row));
        const auto position = runtime.AbsolutePosition(target);
        const auto size = runtime.WindowSize(target);
        MagicWindowSyntheticClick click;
        click.target = target;
        click.cursor = {position.x + size.x * 0.5f,
                        position.y + size.y * 0.5f};
        runtime.DispatchStuntClick(state.panel_owner, click);
        return true;
    }
    default:
        return HandleEvent(member, event_code, state, panel, runtime);
    }
}

bool MagicWindowPanelEvents::ResetHighlight(
    MagicWindowPanelEventState& state, const MagicWindowPanelState& panel,
    IMagicWindowPanelEventsRuntime& runtime) {
    SetHighlight(false, state, panel, runtime);
    return true;
}

void MagicWindowPanelEvents::SetHighlight(
    bool requested, MagicWindowPanelEventState& state,
    const MagicWindowPanelState& panel,
    IMagicWindowPanelEventsRuntime& runtime) {
    const auto primary = panel.At(0x80u);
    const auto secondary = panel.At(0x84u);
    state.highlight_active = requested;
    if (!runtime.IsVisible(primary)) state.highlight_active = false;

    if (requested) {
        if (runtime.IsVisible(secondary)) {
            runtime.SetHoverFlag(secondary, true);
            runtime.RequestRedraw(secondary);
        }
    } else {
        runtime.SetHoverFlag(secondary, false);
        runtime.RequestRedraw(secondary);
        runtime.SetHoverFlag(primary, false);
        runtime.RequestRedraw(primary);
    }
}

}  // namespace pal4
