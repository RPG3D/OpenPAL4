#include "pal4/role_state_window_selection.h"

#include <array>

namespace pal4 {

void RoleStateWindowSelection::SetRoleState(
    RoleStateWindowState& state,
    IRoleStateWindowSelectionRuntime& runtime) {
    state.Write(4u, 1u);
    const auto background = state.Read(96u);
    runtime.SetVisible(background, true);
    runtime.SetImage(background, "kuang0", "huangse");

    constexpr std::array<std::string_view, 4> kButtons{{
        "renwuanniu2", "renwuanniu2b2",
        "renwuanniu2c2", "renwuanniu2d2"
    }};
    const auto role_index = state.Read(0u);
    if (role_index < kButtons.size()) {
        runtime.SetImage(state.Read(100u), "kuang0",
                         kButtons[role_index]);
    }
}

void RoleStateWindowSelection::SetButtonImage(
    RoleStateWindowState& state,
    IRoleStateWindowSelectionRuntime& runtime) {
    state.Write(4u, 0u);
    const auto background = state.Read(96u);
    runtime.SetVisible(background, true);
    runtime.ClearImage(background);

    constexpr std::array<std::string_view, 4> kButtons{{
        "renwuanniu1", "renwuanniub",
        "renwuanniuc", "renwuanniud"
    }};
    const auto role_index = state.Read(0u);
    if (role_index < kButtons.size()) {
        runtime.SetImage(state.Read(100u), "kuang0",
                         kButtons[role_index]);
    }
}

void RoleStateWindowSelection::SetHoveredButtonImage(
    RoleStateWindowState& state,
    IRoleStateWindowSelectionRuntime& runtime) {
    state.Write(4u, 2u);
    const auto background = state.Read(96u);
    runtime.SetVisible(background, true);
    runtime.SetImage(background, "kuang0", "huose");

    constexpr std::array<std::string_view, 4> kButtons{{
        "renwuanniu3", "renwuanniu3b3",
        "renwuanniu3c3", "renwuanniu3d3"
    }};
    const auto role_index = state.Read(0u);
    if (role_index < kButtons.size()) {
        runtime.SetImage(state.Read(100u), "kuang0",
                         kButtons[role_index]);
    }
}

}  // namespace pal4
