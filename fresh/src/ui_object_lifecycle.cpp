#include "pal4/ui_object_lifecycle.h"

namespace pal4 {

UiObjectState& UiObjectLifecycle::Construct(
    UiObjectState& state, const std::uint32_t layout,
    IUiObjectLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, layout);
    try {
        runtime.ConstructMember(state);
    } catch (...) {
        runtime.DestroyLayout(state);
        throw;
    }
    state.Write(0x54u, UiObjectState::kTailVtable);
    state.Write(0u, UiObjectState::kVtable);
    state.Write(0x28u, UiObjectState::kMemberVtable);
    try {
        runtime.InitializeElements(state);
    } catch (...) {
        runtime.DestroyMember(state);
        runtime.DestroyLayout(state);
        throw;
    }
    return state;
}

void UiObjectLifecycle::CleanupMember(
    UiObjectState& state, IUiObjectLifecycleRuntime& runtime) {
    runtime.FreeMemberBuffer(state.Read(0x30u));
    state.Write(0x30u, 0u);
    state.Write(0x34u, 0u);
    state.Write(0x38u, 0u);
}

}  // namespace pal4
