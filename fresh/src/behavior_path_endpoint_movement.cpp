#include "pal4/behavior_path_endpoint_movement.h"
#include "pal4/behavior_path_endpoint_distance.h"

namespace pal4 {
namespace {

std::uint8_t ReadByte(const BehaviorPathEndpointState& state,
                      std::uint32_t byte_offset) {
    const auto shift = 8u * (byte_offset & 3u);
    return static_cast<std::uint8_t>(state.words[byte_offset / 4u] >> shift);
}

void WriteByte(BehaviorPathEndpointState& state, std::uint32_t byte_offset,
               std::uint8_t value) {
    const auto shift = 8u * (byte_offset & 3u);
    const auto mask = 0xffu << shift;
    auto& word = state.words[byte_offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint32_t BehaviorPathMovementRuntimeStub::ProcessCollision(
    BehaviorPathEndpointState&, BehaviorContext3&,
    std::uint32_t, float) { return 0u; }
void BehaviorPathMovementRuntimeStub::UpdateBase(
    BehaviorPathEndpointState&, std::uint32_t, float) {}
BehaviorPositionResult BehaviorPathMovementRuntimeStub::ReadCurrentPosition(
    BehaviorPathEndpointState&, BehaviorContext3&) { return {}; }
float BehaviorPathMovementRuntimeStub::ReadPathRadius(std::uint32_t) {
    return 0.0f;
}
std::uint32_t BehaviorPathMovementRuntimeStub::Read32(std::uint32_t) {
    return 0u;
}
std::uint32_t BehaviorPathMovementRuntimeStub::NotifyTargetSlot04(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t BehaviorPathEndpointMovement::Update(
    BehaviorPathEndpointState& state, std::uint32_t input,
    float delta, IBehaviorPathMovementRuntime& runtime) {
    if (state.words[47] == 0u) {
        WriteByte(state, 0xb9u, 0u);
        return 0u;
    }

    BehaviorContext3 context{};
    const auto collision_result = runtime.ProcessCollision(
        state, context, input, delta);
    runtime.UpdateBase(state, collision_result, delta);
    if (ReadByte(state, 0xc5u) != 0u) {
        const auto first = runtime.ReadCurrentPosition(state, context);
        if (BehaviorPathEndpointDistance::WithinRadius(
                state, 50u, first.value,
                runtime.ReadPathRadius(state.words[47])))
            state.words[48] = 1u;

        const auto last = runtime.ReadCurrentPosition(state, context);
        if (BehaviorPathEndpointDistance::WithinRadius(
                state, 53u, last.value,
                runtime.ReadPathRadius(state.words[47])))
            state.words[48] = 0xffffffffu;
    }

    const auto captured_vtable = runtime.Read32(state.words[42]);
    const auto position = runtime.ReadCurrentPosition(state, context);
    const auto result = runtime.NotifyTargetSlot04(
        captured_vtable, state.words[42], position.address);
    WriteByte(state, 0xb9u, 1u);
    return result;
}

std::uint32_t BehaviorPathCallbackRuntimeStub::InvokeCallback(
    std::uint32_t, BehaviorPathEndpointState&, std::uint32_t) { return 0u; }
std::uint32_t BehaviorPathCallbackRuntimeStub::DeleteObject(
    std::uint32_t, std::uint32_t) { return 0u; }
void BehaviorPathCallbackRuntimeStub::ReleaseAssociation(
    std::uint32_t, std::uint32_t) {}
std::uint32_t BehaviorPathCallbackRuntimeStub::BindAssociation(
    std::uint32_t, BehaviorPathEndpointState&) { return 0u; }

std::uint32_t BehaviorPathEndpointCallbacks::Call(
    BehaviorPathEndpointState& state, std::uint32_t argument,
    IBehaviorPathCallbackRuntime& runtime) {
    const auto callback = state.words[44];
    if (callback == 0u) return 0u;
    return runtime.InvokeCallback(callback, state, argument);
}

std::uint32_t BehaviorPathEndpointCallbacks::DeleteObject(
    std::uint32_t object, std::uint32_t incoming_eax,
    IBehaviorPathCallbackRuntime& runtime) {
    if (object == 0u) return incoming_eax;
    return runtime.DeleteObject(object, 1u);
}

std::uint32_t BehaviorPathEndpointCallbacks::SetAssociation(
    BehaviorPathEndpointState& state, std::uint32_t binder,
    IBehaviorPathAssociationRuntime& runtime) {
    if (state.words[42] != 0u) {
        runtime.ReleaseAssociation(state.words[42], 1u);
        // Native services may throw while creating the replacement.
        state.words[42] = 0u;
    }
    const auto result = runtime.BindAssociation(binder, state);
    state.words[42] = result;
    return result;
}

}  // namespace pal4
