#include "pal4/role_state_window_events.h"
#include "pal4/role_state_window_members.h"
#include "pal4/role_state_window_selection.h"

namespace pal4 {
namespace {

void ResetUnhovered(RoleStateWindowState& state,
                    IRoleStateWindowEventRuntime& runtime) {
    for (std::uint32_t i = 0u; i < RoleStateWindowMembers::kCount; ++i) {
        auto member = RoleStateWindowMembers::Load(state, i);
        if (member.Read(4u) != 2u) {
            RoleStateWindowSelection::SetButtonImage(member, runtime);
            RoleStateWindowMembers::Store(state, i, member);
        }
    }
}

void Select(RoleStateWindowState& state, const std::uint32_t index,
            IRoleStateWindowEventRuntime& runtime) {
    auto member = RoleStateWindowMembers::Load(state, index);
    RoleStateWindowSelection::SetRoleState(member, runtime);
    RoleStateWindowMembers::Store(state, index, member);
}

}  // namespace

bool RoleStateWindowEvents::OnEvent(
    RoleStateWindowState& state, const std::uint32_t owner,
    const std::uint32_t event, IRoleStateWindowEventRuntime& runtime) {
    const auto code = runtime.ReadWord(event + 0x10u);
    if (code == 1u) {
        runtime.Close(owner);
        runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                             kCloseEvent, 1u, 0u);
        return true;
    }
    if (static_cast<std::int32_t>(runtime.ActivePlayerCount(
            runtime.PlayerManager())) < 2)
        return false;

    if (code == 203u) {
        ResetUnhovered(state, runtime);
        auto index = state.Read(588u);
        if (static_cast<std::int32_t>(index) > 0) {
            --index;
            state.Write(588u, index);
        }
        Select(state, index, runtime);
        return true;
    }
    if (code == 205u) {
        const auto index = state.Read(588u);
        if (static_cast<std::int32_t>(index + 1u) < 4) {
            const auto next = index + 1u;
            auto member = RoleStateWindowMembers::Load(state, next);
            if (!RoleStateWindowInteraction::IsVisible(member, runtime))
                return true;
            if (RoleStateWindowInteraction::IsVisible(member, runtime)) {
                ResetUnhovered(state, runtime);
                state.Write(588u, next);
                Select(state, next, runtime);
            }
        }
        return true;
    }
    if (code == 28u) {
        const auto index = state.Read(588u);
        runtime.HandleMouseEvent(state.Read(
            RoleStateWindowMembers::Offset(index) + 12u));
        return true;
    }
    return false;
}

bool RoleStateWindowEvents::UpdateButtonImages(
    RoleStateWindowState& state,
    IRoleStateWindowSelectionRuntime& runtime) {
    for (std::uint32_t i = 0u; i < RoleStateWindowMembers::kCount; ++i) {
        auto member = RoleStateWindowMembers::Load(state, i);
        if (member.Read(4u) == 1u) {
            RoleStateWindowSelection::SetButtonImage(member, runtime);
            RoleStateWindowMembers::Store(state, i, member);
        }
    }
    return true;
}

}  // namespace pal4
