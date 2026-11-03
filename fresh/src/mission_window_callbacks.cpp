#include "pal4/mission_window_callbacks.h"

namespace pal4 {

std::uint8_t MissionWindowCallbacks::MissionClicked(
    MissionWindowState& state, std::uint32_t,
    IMissionWindowCallbacksRuntime& runtime) {
    (void)OpenMission(state, runtime);
    return 1u;
}

std::uint8_t MissionWindowCallbacks::OpenMission(
    MissionWindowState& state, IMissionWindowCallbacksRuntime& runtime) {
    runtime.InsertActiveMember(state, 0x40u);
    runtime.PopulateMissionList(state);
    return 1u;
}

std::uint8_t MissionWindowCallbacks::QuestClicked(
    MissionWindowState& state, std::uint32_t,
    IMissionWindowCallbacksRuntime& runtime) {
    (void)OpenQuest(state, runtime);
    return 1u;
}

std::uint8_t MissionWindowCallbacks::OpenQuest(
    MissionWindowState& state, IMissionWindowCallbacksRuntime& runtime) {
    runtime.InsertActiveMember(state, 0x40u);
    runtime.PopulateQuestList(state);
    runtime.SetVisible(state.Read(0x84u), false);
    runtime.SetAlwaysOnTop(state.Read(0x54u), true);
    return 1u;
}

std::uint8_t MissionWindowCallbacks::HoverChanged(
    MissionWindowState& state, std::uint32_t,
    IMissionWindowDetailsCallbackRuntime& runtime) {
    runtime.UpdateDetailsPanel(state);
    return 1u;
}

std::uint8_t MissionWindowCallbacks::ListMouseClick(
    MissionWindowState& state, std::uint32_t,
    IMissionWindowListCallbackRuntime& runtime) {
    runtime.UpdateDetailsPanel(state);
    if (state.bytes[0x88u] != 0u) {
        runtime.InsertActiveMember(state, 0x40u);
    } else {
        runtime.InsertActiveMember(state, 0x48u);
        const auto mask = state.Read(0x84u);
        runtime.SetVisible(mask, true);
        runtime.SetAlwaysOnTop(state.Read(0x54u), false);
        runtime.HandleMouseEvent(mask);
    }
    return 1u;
}

}  // namespace pal4
