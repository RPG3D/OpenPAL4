#include "pal4/pal_actor_control_actions.h"
#include "pal4/actor_control_world_movement.h"

#include <cstring>

namespace pal4 {

std::uint32_t IActorControlActionRuntime::UpdateCameraAndWorldView(
    std::uint32_t*, std::uint32_t actor,
    std::uint32_t position_address) {
    ActorControlMovementGlobals globals{};
    ActorControlDiscreteMovementRuntimeStub runtime;
    return ActorControlWorldMovement::ApplyDelta(
        actor, position_address, globals, runtime);
}
std::uint32_t IActorControlActionRuntime::MovementAdapterState() {
    return 0u;
}

std::uint32_t PalActorControlActions::BeginMotion(
    PalActorControlState& state, const std::uint32_t base_address,
    const std::array<std::uint32_t, 3>& start,
    const std::array<std::uint32_t, 3>& finish,
    const std::uint32_t alternate_height_bits, const std::uint32_t reached,
    IActorControlMotionRuntime& runtime) {
    ActorControlMotionState motion{};
    std::memcpy(&motion, state.words.data() + 165u, sizeof(motion));
    std::uint32_t result{};
    try {
        result = ActorControlMotion::Begin(
            motion, start, finish, alternate_height_bits, reached, runtime);
    } catch (...) {
        std::memcpy(state.words.data() + 165u, &motion, sizeof(motion));
        throw;
    }
    std::memcpy(state.words.data() + 165u, &motion, sizeof(motion));
    state.words[183u] = base_address + 0x294u;
    return result;
}

std::uint32_t PalActorControlActions::ClearTargets(
    PalActorControlState& state) {
    state.words[183u] = 0u;
    state.words[20u] = 0u;
    state.words[21u] = 0u;
    state.words[22u] = 0u;
    return 0u;
}

std::uint32_t PalActorControlActions::UpdateWorldView(
    PalActorControlState& state, const std::uint32_t position_address,
    IActorControlActionRuntime& runtime) {
    const auto actor = state.words[1u];
    if (actor == 0u) {
        return actor;
    }
    return runtime.UpdateCameraAndWorldView(
        state.words.data() + 23u, actor, position_address);
}

std::uint32_t PalActorControlActions::SetWord28(
    PalActorControlState& state, const std::uint32_t value) {
    state.words[28u] = value;
    return value;
}

bool PalActorControlActions::CanStartAction(
    const PalActorControlState& state,
    IActorControlActionRuntime& runtime) {
    return state.words[1u] != 0u && state.words[183u] == 0u &&
        runtime.MovementAdapterState() == 0u;
}

}  // namespace pal4
