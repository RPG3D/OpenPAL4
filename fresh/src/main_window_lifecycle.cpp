#include "pal4/main_window_lifecycle.h"

namespace pal4 {

MainWindowState& MainWindowLifecycle::Construct(
    MainWindowState& state, std::uint32_t name,
    IMainWindowLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    try {
        runtime.ConstructMember(state, 0x28u);
        try {
            state.Write(0x28u, kBaseMemberVtable);
            state.Write(0u, kObjectVtable);
            state.Write(0x28u, kMemberVtable);
            for (std::uint32_t offset = 0x40u;
                 offset <= 0x60u; offset += 4u) {
                state.Write(offset, 0u);
            }
            state.bytes[0x68u] = 1u;
            runtime.InitializeButtons(state);
        } catch (...) {
            runtime.CleanupMember(state, 0x28u);
            throw;
        }
    } catch (...) {
        runtime.DestroyDialog(state);
        throw;
    }
    return state;
}

std::uint32_t MainWindowLifecycle::Destroy(
    MainWindowState& state, std::uint32_t object_address,
    std::uint32_t flags, IMainWindowLifecycleRuntime& runtime) {
    runtime.DestroyTimerFrame(state);
    if ((flags & 1u) != 0u) runtime.FreeObject(object_address);
    return object_address;
}

}  // namespace pal4
