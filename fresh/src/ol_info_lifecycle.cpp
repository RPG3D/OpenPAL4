#include "pal4/ol_info_lifecycle.h"

namespace pal4 {

OlInfoState& OlInfoLifecycle::Construct(
    OlInfoState& state, std::uint32_t name,
    IOlInfoLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    try {
        state.Write(0x28u, kBaseMemberVtable);
        state.Write(0u, kObjectVtable);
        state.Write(0x28u, kMemberVtable);
        runtime.InitializePanel(state);
    } catch (...) {
        runtime.DestroyDialog(state);
        throw;
    }
    return state;
}

}  // namespace pal4
