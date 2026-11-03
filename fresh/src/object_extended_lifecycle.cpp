#include "pal4/object_extended_lifecycle.h"

namespace pal4 {

void ObjectExtendedRuntimeStub::ConstructBase(ObjectMovementState&) {}
void ObjectExtendedRuntimeStub::CleanupBase(ObjectMovementState&) {}
void ObjectExtendedRuntimeStub::Free(std::uint32_t) {}
void ObjectExtendedRuntimeStub::CallCapturedSlotB0(
    std::uint32_t, std::uint32_t) {}
std::uint32_t ObjectExtendedRuntimeStub::CallCurrentSlotA0(
    ObjectMovementState&) { return 0u; }

ObjectMovementState& ObjectExtendedLifecycle::Construct(
    ObjectMovementState& state, IObjectExtendedRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0] = kVtable;
    state.words[47] = 0u;
    return state;
}

void ObjectExtendedLifecycle::Cleanup(
    ObjectMovementState& state, IObjectExtendedRuntime& runtime) {
    state.words[0] = kVtable;
    state.words[47] = 0u;
    runtime.CleanupBase(state);
}

std::uint32_t ObjectExtendedLifecycle::Delete(
    ObjectMovementState& state, std::uint32_t object_address,
    std::uint32_t flags, IObjectExtendedRuntime& runtime) {
    Cleanup(state, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object_address);
    return object_address;
}

std::uint32_t ObjectExtendedLifecycle::SetPropertyAndDispatch(
    ObjectMovementState& state, std::uint32_t first,
    std::uint32_t property, IObjectExtendedRuntime& runtime) {
    const auto captured_vtable = state.words[0];
    state.words[47] = property;
    runtime.CallCapturedSlotB0(captured_vtable, first);
    return runtime.CallCurrentSlotA0(state);
}

}  // namespace pal4
