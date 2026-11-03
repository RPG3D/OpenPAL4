#include "pal4/mission_window_visibility.h"

namespace pal4 {

void MissionWindowVisibility::SetCharacterPanel(
    MissionWindowState& state, bool visible,
    IMissionWindowPanelRuntime& runtime) {
    runtime.SetVisible(state.Read(0x70u), visible);
    runtime.SetVisible(state.Read(0x78u), visible);
    runtime.SetVisible(state.Read(0x6cu), visible);
    runtime.SetVisible(state.Read(0x74u), visible);
    runtime.SetImage(state.Read(0x7cu), "zi0", "renwu");
}

void MissionWindowVisibility::SetMissionPanel(
    MissionWindowState& state, bool visible,
    IMissionWindowPanelRuntime& runtime) {
    runtime.SetVisible(state.Read(0x70u), visible);
    runtime.SetVisible(state.Read(0x78u), visible);
    runtime.SetVisible(state.Read(0x6cu), !visible);
    runtime.SetVisible(state.Read(0x74u), true);
}

void MissionWindowVisibility::Set(
    MissionWindowState& state, std::uint8_t visible,
    std::uint32_t parent, IMissionWindowVisibilityRuntime& runtime) {
    runtime.SetParent(state, visible, parent);
    runtime.ProcessUiEvent(0x8b1394u, visible, 0u);
    runtime.ProcessUiEvent(0x8b13a0u, visible, state.Read(0x24u));
    if (visible != 0u) {
        runtime.ProcessUiEvent(0x8b13a4u, 12u, 0u);
        runtime.SetVisible(state.Read(0x84u), false);
        runtime.SetAlwaysOnTop(state.Read(0x54u), true);
    }
    SetCharacterPanel(state, false, runtime);
    runtime.SetListHeaderByte(state.Read(0x58u), 1239u, 0u);
    runtime.SetListHeaderByte(state.Read(0x54u), 1239u, 0u);
    runtime.SetSelected(state.Read(0x64u), false);
    runtime.SetSelected(state.Read(0x60u), false);
    if (visible != 0u) {
        runtime.InsertActiveMember(state, 0x28u);
        runtime.InsertActiveMember(state, 0x40u);
        runtime.PopulateMissionList(state);
        runtime.UpdateSystemDialogVisibility(6u);
    } else {
        runtime.RemoveActiveMember(state, 0x28u);
    }
}

}  // namespace pal4
