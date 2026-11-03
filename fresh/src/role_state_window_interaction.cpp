#include "pal4/role_state_window_interaction.h"

namespace pal4 {

bool RoleStateWindowInteraction::OnMouseMove(
    RoleStateWindowState& state, const std::uint32_t event,
    IRoleStateWindowInteractionRuntime& runtime) {
    if (state.Read(4u) != 0u) return true;
    const auto receiver = state.Read(116u);
    if (static_cast<std::int32_t>(
            runtime.ReadWord(receiver + 0x254u)) < 2)
        return true;

    const auto event_window = runtime.ReadWord(event + 8u);
    const auto captured = runtime.CapturedWindow();
    if (captured != 0u && captured != event_window) return true;
    const auto root = runtime.RootWindow();
    if (root != 0u &&
        runtime.ChildAtPosition(root, event + 12u) == event_window)
        RoleStateWindowSelection::SetRoleState(state, runtime);
    return true;
}

bool RoleStateWindowInteraction::OnMouseLeave(
    RoleStateWindowState& state,
    IRoleStateWindowInteractionRuntime& runtime) {
    if (state.Read(4u) == 1u)
        RoleStateWindowSelection::SetButtonImage(state, runtime);
    return true;
}

void RoleStateWindowInteraction::SetVisible(
    RoleStateWindowState& state, const bool visible,
    IRoleStateWindowSelectionRuntime& runtime) {
    runtime.SetVisible(state.Read(12u), visible);
    runtime.SetVisible(state.Read(72u), visible);
    runtime.SetVisible(state.Read(76u), !visible);
}

bool RoleStateWindowInteraction::IsVisible(
    const RoleStateWindowState& state,
    IRoleStateWindowInteractionRuntime& runtime) {
    return runtime.IsVisible(state.Read(12u), false);
}

}  // namespace pal4
