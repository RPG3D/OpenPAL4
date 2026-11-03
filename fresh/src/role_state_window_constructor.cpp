#include "pal4/role_state_window_constructor.h"
#include "pal4/role_state_window_members.h"

namespace pal4 {
namespace {

void Bind(IRoleStateWindowConstructorRuntime& runtime,
          const std::uint32_t window, const RoleMouseEvent event,
          const std::uint32_t handler, const std::uint32_t owner) {
    const auto callback = runtime.Allocate(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 16u, owner);
        runtime.WriteWord(callback + 8u, handler);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback, RoleStateWindowConstructor::kCallbackVtable);
    }
    const auto connection = runtime.Subscribe(window, event, callback);
    if (connection != 0u) runtime.ReleaseConnection(connection);
}

}  // namespace

std::uint32_t RoleStateWindowConstructor::Construct(
    RoleStateWindowState& state, const std::uint32_t owner,
    const std::uint32_t name, IRoleStateWindowInitRuntime& init_runtime,
    IRoleStateWindowConstructorRuntime& runtime) {
    runtime.LoadLayout(state, name);
    state.Write(0u, kObjectVtable);
    state.Write(40u, kDialogVtable);
    state.Write(592u, 0u);

    for (std::uint32_t role = 0u;
         role < RoleStateWindowMembers::kCount; ++role) {
        RoleStateWindowState member{};
        static_cast<void>(RoleStateWindowInitializer::Initialize(
            member, owner, role, 0u, owner, init_runtime));
        RoleStateWindowMembers::Store(state, role, member);

        Bind(runtime, state.Read(RoleStateWindowMembers::Offset(role) + 12u),
             RoleMouseEvent::ButtonUp, kButtonUpHandler, owner);
        Bind(runtime, state.Read(RoleStateWindowMembers::Offset(role) + 96u),
             RoleMouseEvent::ButtonUp, kButtonUpHandler, owner);
    }
    Bind(runtime, state.Read(36u), RoleMouseEvent::Move,
         kButtonMoveHandler, owner);
    return owner;
}

}  // namespace pal4
