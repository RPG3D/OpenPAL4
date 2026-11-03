#include "pal4/actor_control_input_component.h"
#include "pal4/actor_control_movement_mode.h"
#include "pal4/actor_control_input_events.h"
#include "pal4/actor_control_free_movement.h"

#include <bit>

namespace pal4 {

std::int32_t ActorControlInputComponentRuntimeStub::GameConfigPrimary() {
    return 1;
}

void ActorControlInputComponentRuntimeStub::SetAnimationFlag(
    std::uint32_t, std::uint32_t) {}
void ActorControlInputComponentRuntimeStub::HandleInputEvents(
    ActorControlInputComponentState& state, std::uint32_t actor,
    float delta_seconds) {
    ActorControlInputEventRuntimeStub runtime;
    static_cast<void>(ActorControlInputComponent::HandleInputEvents(
        state, actor, delta_seconds, runtime));
}
void ActorControlInputComponentRuntimeStub::UpdateInputObject(
    ActorControlInputComponentState& state, std::uint32_t actor,
    float delta_seconds) {
    ActorControlFreeMovementRuntimeStub runtime;
    static_cast<void>(ActorControlInputComponent::UpdateFreeMovement(
        state, actor, delta_seconds, runtime));
}

ActorControlInputComponentState& ActorControlInputComponent::Construct(
    ActorControlInputComponentState& state,
    IActorControlInputComponentRuntime& runtime) {
    auto& words = state.words;
    words[0] = kVtable;
    words[1] = 0x4329e666u;
    words[2] = 0x42fdcccdu;
    const auto config = runtime.GameConfigPrimary();
    words[5] = 1u;
    words[4] = 0u;
    words[8] = 0u;
    words[7] = 0u;
    words[6] = 0u;
    words[9] = 0u;
    words[10] = 1u;
    words[11] = 0x3f800000u;
    words[1] = 0x4329e666u;
    words[3] = std::bit_cast<std::uint32_t>(
        static_cast<float>(270.0 / static_cast<double>(config)));
    return state;
}

std::uint32_t ActorControlInputComponent::Initialize(
    ActorControlInputComponentState& state) {
    state.words[4] = 0u;
    return 1u;
}

std::uint32_t ActorControlInputComponent::ResetActor(
    ActorControlInputComponentState& state, std::uint32_t actor,
    IActorControlInputComponentRuntime& runtime) {
    state.words[4] = 0u;
    state.words[9] = 0u;
    if (actor != 0u)
        runtime.SetAnimationFlag(actor + 0x170u, 0x3f800000u);
    return 1u;
}

std::uint32_t ActorControlInputComponent::ResetActorWithoutAnimation(
    ActorControlInputComponentState& state, std::uint32_t) {
    state.words[4] = 0u;
    state.words[9] = 0u;
    return 1u;
}

std::uint32_t ActorControlInputComponent::ProcessInput(
    ActorControlInputComponentState& state, std::uint32_t actor,
    float delta_seconds, IActorControlInputComponentRuntime& runtime) {
    const auto event_mode = state.words[5];
    state.words[4] = 0u;
    if (event_mode != 0u)
        runtime.HandleInputEvents(state, actor, delta_seconds);
    else
        runtime.UpdateInputObject(state, actor, delta_seconds);
    return state.words[4];
}

std::uint32_t ActorControlInputComponent::SetMovementMode(
    ActorControlInputComponentState& state, std::uint32_t mode,
    std::uint32_t) {
    state.words[10] = mode;
    auto multiplier=std::bit_cast<float>(state.words[11]);float speed{};
    const auto result=ApplyActorMovementMode(mode,multiplier,speed);
    state.words[11]=std::bit_cast<std::uint32_t>(multiplier);
    state.words[1]=std::bit_cast<std::uint32_t>(speed);
    return result;
}

}  // namespace pal4
