#include "pal4/manager_object_dispatch.h"

namespace pal4 {

std::uint32_t ManagerObjectDispatchRuntimeStub::Invoke(
    std::uint32_t /*target*/, std::uint32_t /*slot_offset*/) {
    return 0u;
}

std::uint8_t ManagerObjectDispatch::Invoke(
    const ManagerObjectDispatchState& state, const std::uint32_t target,
    const std::uint32_t slot_offset,
    IManagerObjectDispatchRuntime& runtime) {
    if (state.initialized == 0u) {
        return state.initialized;
    }
    if (state.enabled == 0u) {
        return state.enabled;
    }
    return static_cast<std::uint8_t>(runtime.Invoke(target, slot_offset));
}

std::uint8_t ManagerObjectDispatch::Begin(
    const ManagerObjectDispatchState& state,
    IManagerObjectDispatchRuntime& runtime) {
    return Invoke(state, state.primary_target, 284u, runtime);
}

std::uint8_t ManagerObjectDispatch::End(
    const ManagerObjectDispatchState& state,
    IManagerObjectDispatchRuntime& runtime) {
    return Invoke(state, state.primary_target, 288u, runtime);
}

std::uint8_t ManagerObjectDispatch::BeginSecondary(
    const ManagerObjectDispatchState& state,
    IManagerObjectDispatchRuntime& runtime) {
    return Invoke(state, state.secondary_target, 284u, runtime);
}

std::uint8_t ManagerObjectDispatch::EndSecondary(
    const ManagerObjectDispatchState& state,
    IManagerObjectDispatchRuntime& runtime) {
    return Invoke(state, state.secondary_target, 288u, runtime);
}

std::uint8_t ManagerObjectDispatch::ReturnFalse(
    std::uint32_t /*first*/, std::uint32_t /*second*/) {
    return 0u;
}

}  // namespace pal4
