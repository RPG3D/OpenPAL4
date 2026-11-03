#include "pal4/actor_control_input_events.h"

#include "pal4/actor_control_discrete_movement.h"

#include <bit>

namespace pal4 {

std::uint32_t ActorControlInputEventRuntimeStub::InputManager() {
    return 0u;
}
std::uint32_t ActorControlInputEventRuntimeStub::ReadInputWord(
    std::uint32_t, std::uint32_t) { return 0u; }
ActorControlScreenPoint ActorControlInputEventRuntimeStub::CursorPoint() {
    return {};
}
ActorControlScreenRect ActorControlInputEventRuntimeStub::WindowRect() {
    return {};
}
void ActorControlInputEventRuntimeStub::GetInputStateValues(
    std::uint32_t, std::uint32_t&, std::uint32_t&) {}
void ActorControlInputEventRuntimeStub::DispatchMovement(
    ActorControlInputComponentState& state, std::uint32_t actor,
    std::uint32_t command, float delta_seconds) {
    ActorControlMovementGlobals globals{};
    ActorControlDiscreteMovementRuntimeStub runtime;
    static_cast<void>(ActorControlInputComponent::DispatchDiscreteMovement(
        state, globals, actor, command, delta_seconds, runtime));
}
void ActorControlInputEventRuntimeStub::TurnFromMouse(
    ActorControlInputComponentState& state, std::uint32_t actor,
    std::int32_t input_value) {
    ActorControlDiscreteMovementRuntimeStub runtime;
    static_cast<void>(ActorControlInputComponent::ApplyMouseTurnValue(
        state, actor, input_value, runtime));
}

namespace {

bool InputActive(IActorControlInputEventRuntime& runtime,
                 std::uint32_t count_offset, std::uint32_t flag_offset) {
    const auto manager = runtime.InputManager();
    const auto count = std::bit_cast<std::int32_t>(
        runtime.ReadInputWord(manager, count_offset));
    if (count <= 0) return false;
    return (runtime.ReadInputWord(manager, flag_offset) & 0xfu) != 0u;
}

bool CursorInside(IActorControlInputEventRuntime& runtime) {
    const auto point = runtime.CursorPoint();
    const auto rect = runtime.WindowRect();
    return point.x > rect.left && point.x < rect.right &&
           point.y > rect.top && point.y < rect.bottom;
}

void Dispatch(ActorControlInputComponentState& state,
              std::uint32_t actor, std::uint32_t command,
              float delta_seconds, bool clear_pending,
              IActorControlInputEventRuntime& runtime) {
    runtime.DispatchMovement(state, actor, command, delta_seconds);
    state.words[4] = 1u;
    if (clear_pending) state.words[9] = 0u;
}

}  // namespace

std::uint32_t ActorControlInputComponent::HandleInputEvents(
    ActorControlInputComponentState& state, std::uint32_t actor,
    float delta_seconds, IActorControlInputEventRuntime& runtime) {
    const bool pending_forward = state.words[9] != 0u;
    state.words[4] = 0u;
    if (pending_forward)
        Dispatch(state, actor, 1u, delta_seconds, false, runtime);

    if (InputActive(runtime, 0x1f44u, 0x1f38u) ||
        InputActive(runtime, 0x1e34u, 0x1e28u)) {
        Dispatch(state, actor, 1u, delta_seconds, true, runtime);
    } else if (InputActive(runtime, 0x21d4u, 0x21c8u)) {
        if (CursorInside(runtime))
            Dispatch(state, actor, 1u, delta_seconds, true, runtime);
    } else if (InputActive(runtime, 0x1f54u, 0x1f48u) ||
               InputActive(runtime, 0x1df4u, 0x1de8u)) {
        Dispatch(state, actor, 2u, delta_seconds, true, runtime);
    }

    if (InputActive(runtime, 0x1f24u, 0x1f18u) ||
        InputActive(runtime, 0x1cd4u, 0x1cc8u)) {
        Dispatch(state, actor, 3u, delta_seconds, false, runtime);
        return state.words[4];
    }
    if (InputActive(runtime, 0x1f34u, 0x1f28u) ||
        InputActive(runtime, 0x1d04u, 0x1cf8u)) {
        Dispatch(state, actor, 4u, delta_seconds, false, runtime);
        return state.words[4];
    }
    if (InputActive(runtime, 0x21e4u, 0x21d8u) &&
        CursorInside(runtime)) {
        auto input_bits = std::bit_cast<std::uint32_t>(delta_seconds);
        auto scratch_actor = actor;
        runtime.GetInputStateValues(runtime.InputManager(), input_bits,
                                    scratch_actor);
        runtime.TurnFromMouse(state, actor,
                              std::bit_cast<std::int32_t>(input_bits));
        state.words[4] = 1u;
        return 1u;
    }
    return state.words[4];
}

}  // namespace pal4
