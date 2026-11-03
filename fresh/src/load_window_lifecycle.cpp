#include "pal4/load_window_lifecycle.h"

namespace pal4 {

LoadWindowState& LoadWindowLifecycle::Construct(
    LoadWindowState& state, std::uint32_t name,
    ILoadWindowLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    state.Write(0x28u, kBaseMemberVtable);
    try {
        state.Write(0u, kObjectVtable);
        state.Write(0x28u, kMemberVtable);
        runtime.InitializeListElements(state);
    } catch (...) {
        (void)runtime.DestroyDialog(state);
        throw;
    }
    return state;
}

std::uint32_t LoadWindowLifecycle::Destroy(
    LoadWindowState& state, ILoadWindowLifecycleRuntime& runtime) {
    return runtime.DestroyDialog(state);
}

}  // namespace pal4
