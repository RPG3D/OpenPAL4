#include "pal4/role_state_window_player_swap.h"
#include "pal4/role_state_window_members.h"

namespace pal4 {

std::uint8_t RoleStateWindowPlayerSwap::Handle(
    RoleStateWindowState& state, const std::uint32_t owner,
    const std::uint32_t target_index, const std::uint32_t selected_index,
    IRoleStateWindowPlayerRuntime& player_runtime,
    IRoleStateWindowSwapRuntime& runtime) {
    if (target_index == selected_index)
        return static_cast<std::uint8_t>(owner);

    const auto property = state.Read(592u);
    if (property != 0u && runtime.ReadWord(property + 0x34u) != 2u) {
        const auto text = runtime.WarningText(0xe4u);
        const auto dialog = runtime.CreateCommonDialog();
        return runtime.ShowDialog(dialog, text, 0x3f000000u,
                                  0x3f000000u, 4u, 0u);
    }

    auto target = RoleStateWindowMembers::Load(state, target_index);
    auto selected = RoleStateWindowMembers::Load(state, selected_index);
    const auto target_role = target.Read(0u);
    const auto selected_role = selected.Read(0u);
    RoleStateWindowPlayerInfo::Update(target, selected_role, player_runtime);
    RoleStateWindowMembers::Store(state, target_index, target);
    RoleStateWindowPlayerInfo::Update(selected, target_role, player_runtime);
    RoleStateWindowMembers::Store(state, selected_index, selected);
    return runtime.PlaySound("wa12.wav", 1u);
}

}  // namespace pal4
