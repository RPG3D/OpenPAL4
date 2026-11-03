#include "pal4/role_state_window_input_event.h"
#include "pal4/role_state_window_members.h"

namespace pal4 {
namespace {

bool BothInputStatesActive(IRoleStateWindowInputRuntime& runtime) {
    auto manager = runtime.InputManager();
    if (static_cast<std::int32_t>(runtime.ReadWord(manager + 0x1ee4u)) <= 0 ||
        (runtime.ReadWord(manager + 0x1ed8u) & 0xfu) == 0u)
        return false;
    manager = runtime.InputManager();
    return static_cast<std::int32_t>(
               runtime.ReadWord(manager + 0x1ef4u)) > 0 &&
           (runtime.ReadWord(manager + 0x1ee8u) & 0xfu) != 0u;
}

}  // namespace

bool RoleStateWindowInputEvent::Handle(
    RoleStateWindowState& state, const std::uint32_t event,
    IRoleStateWindowPlayerRuntime& player_runtime,
    IRoleStateWindowInputRuntime& runtime) {
    const auto code = runtime.ReadWord(event + 0x10u);
    if (code != 5u && code != 10u) return false;
    if (!BothInputStatesActive(runtime)) return true;

    if (code == 5u) {
        runtime.UpdateAndClampCounter(runtime.InventoryManager(), 500u);
        runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                             kGameInfoEvent, 1u, state.Read(36u));
        return true;
    }

    for (std::uint32_t i = 0u; i < RoleStateWindowMembers::kCount; ++i) {
        const auto player = player_runtime.PlayerByIndex(
            player_runtime.PlayerManager(), i);
        if (runtime.ReadWord(player + 0xb08u) != 0u)
            runtime.UpdatePlayerResource(player + 0x654u, 8u, 100u);
    }
    for (std::uint32_t i = 0u; i < RoleStateWindowMembers::kCount; ++i) {
        auto member = RoleStateWindowMembers::Load(state, i);
        if (runtime.IsVisible(member.Read(12u), false)) {
            RoleStateWindowPlayerInfo::Update(
                member, member.Read(0u), player_runtime);
            RoleStateWindowMembers::Store(state, i, member);
        }
    }
    return true;
}

}  // namespace pal4
