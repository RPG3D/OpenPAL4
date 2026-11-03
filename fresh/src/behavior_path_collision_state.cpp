#include "pal4/behavior_path_collision_state.h"
#include "pal4/behavior_path_endpoint_distance.h"

namespace pal4 {
namespace {

bool IsEndpointCheckEnabled(const BehaviorPathEndpointState& state) {
    return ((state.words[49] >> 8u) & 0xffu) != 0u;
}

void CompareEndpoints(BehaviorPathEndpointState& state,
                      const BehaviorVector3& position,
                      std::uint32_t path_address,
                      IBehaviorPathCollisionStateRuntime& runtime) {
    if (BehaviorPathEndpointDistance::WithinRadius(
            state, 50u, position, runtime.ReadPathRadius(path_address)))
        state.words[48] = 1u;
    if (BehaviorPathEndpointDistance::WithinRadius(
            state, 53u, position, runtime.ReadPathRadius(path_address)))
        state.words[48] = 0xffffffffu;
}

}  // namespace

BehaviorVector3 BehaviorPathCollisionStateRuntimeStub::ReadPosition(
    std::uint32_t, BehaviorContext3&) { return {}; }
float BehaviorPathCollisionStateRuntimeStub::ReadPathRadius(
    std::uint32_t) { return 0.0f; }

void BehaviorPathCollisionState::UpdateFromObject(
    BehaviorPathEndpointState& state, std::uint32_t object,
    std::uint32_t, IBehaviorPathCollisionStateRuntime& runtime) {
    if (!IsEndpointCheckEnabled(state)) return;
    BehaviorContext3 scratch{};
    const auto first = runtime.ReadPosition(object, scratch);
    if (BehaviorPathEndpointDistance::WithinRadius(
            state, 50u, first, runtime.ReadPathRadius(state.words[47])))
        state.words[48] = 1u;
    const auto last = runtime.ReadPosition(object, scratch);
    if (BehaviorPathEndpointDistance::WithinRadius(
            state, 53u, last, runtime.ReadPathRadius(state.words[47])))
        state.words[48] = 0xffffffffu;
}

void BehaviorPathCollisionState::UpdateFromVector(
    BehaviorPathEndpointState& state, std::uint32_t, std::uint32_t,
    std::uint32_t, const BehaviorVector3* position,
    IBehaviorPathCollisionStateRuntime& runtime) {
    if (!IsEndpointCheckEnabled(state)) return;
    const auto path_address = state.words[47];
    CompareEndpoints(state, *position, path_address, runtime);
}

void BehaviorPathCollisionState::UpdateFromRecord(
    BehaviorPathEndpointState& state, const std::array<float, 5>* record,
    std::uint32_t, IBehaviorPathCollisionStateRuntime& runtime) {
    if (!IsEndpointCheckEnabled(state)) return;
    const auto path_address = state.words[47];
    const BehaviorVector3 position{(*record)[2], (*record)[3], (*record)[4]};
    CompareEndpoints(state, position, path_address, runtime);
}

}  // namespace pal4
