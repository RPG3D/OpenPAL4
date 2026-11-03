#include "pal4/behavior_path_endpoint_helper.h"

namespace pal4 {

int BehaviorPathGlobalsRuntimeStub::RegisterCleanup() { return 0; }
void BehaviorPathGlobalsRuntimeStub::Free(std::uint32_t) {}

int BehaviorPathEndpointGlobals::RegisterExit(
    BehaviorPathGlobalsState& state, std::uint32_t ecx_value,
    IBehaviorPathGlobalsRuntime& runtime) {
    static_cast<void>(Initialize(state, ecx_value));
    return runtime.RegisterCleanup();
}

int BehaviorPathEndpointGlobals::Initialize(
    BehaviorPathGlobalsState& state, std::uint32_t ecx_value) {
    state.high_ecx_byte = static_cast<std::uint8_t>(ecx_value >> 24u);
    state.begin = 0u;
    state.end = 0u;
    state.capacity = 0u;
    return 0;
}

void BehaviorPathEndpointGlobals::Cleanup(
    BehaviorPathGlobalsState& state, IBehaviorPathGlobalsRuntime& runtime) {
    if ((state.cleanup_guard & 1u) != 0u) return;
    state.cleanup_guard = static_cast<std::uint8_t>(state.cleanup_guard | 1u);
    runtime.Free(state.begin);
    state.begin = 0u;
    state.end = 0u;
    state.capacity = 0u;
}

void BehaviorPathEndpointRuntimeStub::ConstructBase(
    BehaviorPathEndpointState&) {}
void BehaviorPathEndpointRuntimeStub::DestructBase(
    BehaviorPathEndpointState&) {}
void BehaviorPathEndpointRuntimeStub::Free(std::uint32_t) {}
std::uint32_t BehaviorPathEndpointRuntimeStub::Read32(std::uint32_t) {
    return 0u;
}
void BehaviorPathEndpointRuntimeStub::ApplyPathParameter(
    BehaviorPathEndpointState&, std::uint32_t) {}
std::uint32_t BehaviorPathEndpointRuntimeStub::Activate(
    BehaviorPathEndpointState&) { return 0u; }

BehaviorPathEndpointState& BehaviorPathEndpointHelper::Construct(
    BehaviorPathEndpointState& state,
    IBehaviorPathEndpointLifecycleRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[50] = 0u;
    state.words[51] = 0u;
    state.words[52] = 0u;
    state.words[53] = 0u;
    state.words[54] = 0u;
    state.words[55] = 0u;
    state.words[47] = 0u;
    state.words[49] &= 0xffff0000u;
    state.words[0] = kVtable;
    return state;
}

std::uint32_t BehaviorPathEndpointHelper::Delete(
    BehaviorPathEndpointState& state, std::uint32_t address,
    std::uint32_t flags, IBehaviorPathEndpointLifecycleRuntime& runtime) {
    Destruct(state, runtime);
    if ((flags & 1u) != 0u) runtime.Free(address);
    return address;
}

void BehaviorPathEndpointHelper::Destruct(
    BehaviorPathEndpointState& state,
    IBehaviorPathEndpointLifecycleRuntime& runtime) {
    state.words[0] = kVtable;
    state.words[47] = 0u;
    runtime.DestructBase(state);
}

std::uint32_t BehaviorPathEndpointHelper::SetPathAndCacheEndpoints(
    BehaviorPathEndpointState& state, std::uint32_t parameter,
    std::uint32_t path_address, IBehaviorPathEndpointConfigurationRuntime& runtime) {
    state.words[47] = path_address;
    if (path_address != 0u) {
        auto point_address = runtime.Read32(path_address + 0xcu);
        for (std::uint32_t i = 0u; i != 3u; ++i)
            state.words[50u + i] = runtime.Read32(point_address + 4u * i);

        const auto point_count = runtime.Read32(state.words[47] + 8u);
        point_address = runtime.Read32(state.words[47] + 0xcu);
        const auto last_address = point_address + 12u * point_count - 12u;
        for (std::uint32_t i = 0u; i != 3u; ++i)
            state.words[53u + i] = runtime.Read32(last_address + 4u * i);
    }
    runtime.ApplyPathParameter(state, parameter);
    return runtime.Activate(state);
}

}  // namespace pal4
