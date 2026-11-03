#include "pal4/object_derived_lifecycle.h"

namespace pal4 {

ObjectMovementState& ObjectDerivedLifecycle::Construct(
    ObjectMovementState& state, IObjectExtendedRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0] = kVtable;
    return state;
}

void ObjectDerivedLifecycle::Cleanup(
    ObjectMovementState& state, IObjectExtendedRuntime& runtime) {
    state.words[0] = kVtable;
    runtime.CleanupBase(state);
}

std::uint32_t ObjectDerivedLifecycle::Delete(
    ObjectMovementState& state, std::uint32_t object_address,
    std::uint32_t flags, IObjectExtendedRuntime& runtime) {
    Cleanup(state, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object_address);
    return object_address;
}

std::uint32_t ObjectDerivedLifecycle::Dispatch(
    ObjectMovementState& state, std::uint32_t first,
    std::uint32_t /*unused*/, IObjectExtendedRuntime& runtime) {
    runtime.CallCapturedSlotB0(state.words[0], first);
    return runtime.CallCurrentSlotA0(state);
}

}  // namespace pal4
