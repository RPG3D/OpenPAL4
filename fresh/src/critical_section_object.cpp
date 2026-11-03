#include "pal4/critical_section_object.h"

namespace pal4 {

CriticalSectionState& CriticalSectionObject::Initialize(
    CriticalSectionState& state, ICriticalSectionObjectRuntime& runtime) {
    state.handle = 0u;
    state.trailing = 0u;
    state.running = 0u;
    state.vtable = kVtable;
    state.available = 1u;
    runtime.InitializeNative(state);
    return state;
}

void CriticalSectionObject::Destroy(
    CriticalSectionState& state, ICriticalSectionObjectRuntime& runtime) {
    const auto running = state.running;
    state.vtable = kVtable;
    if (running != 0u) (void)WaitAndRelease(state, 300u, runtime);
    runtime.DeleteNative(state);
}

std::uint32_t CriticalSectionObject::WaitAndRelease(
    CriticalSectionState& state, const std::uint32_t milliseconds,
    ICriticalSectionObjectRuntime& runtime) {
    if (state.running == 0u)
        return (runtime.InitialReturnWord() & 0xffffff00u) | state.running;
    const auto result = state.handle;
    if (result == 0u) return result;
    runtime.WaitForSingleObject(result, milliseconds);
    runtime.CloseHandle(state.handle);
    state.handle = 0u;
    state.available = 1u;
    state.running = 0u;
    runtime.LeaveNative(state);
    return result;
}

}  // namespace pal4
