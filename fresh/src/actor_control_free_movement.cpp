#include "pal4/actor_control_free_movement.h"
#include "pal4/actor_control_mouse_projection.h"

#include <bit>

namespace pal4 {

std::uint32_t ActorControlFreeMovementRuntimeStub::InputManager() {
    return 0u;
}
std::uint32_t ActorControlFreeMovementRuntimeStub::ReadInputWord(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t ActorControlFreeMovementRuntimeStub::CameraSystem() {
    return 0u;
}
std::uint32_t ActorControlFreeMovementRuntimeStub::ActiveCameraObjectPointer(
    std::uint32_t) { return 0u; }
std::uint32_t ActorControlFreeMovementRuntimeStub::ReadWord(
    std::uint32_t) { return 0u; }
std::uint32_t ActorControlFreeMovementRuntimeStub::ObjectTransform(
    std::uint32_t) { return 0u; }
bool ActorControlFreeMovementRuntimeStub::MouseWorldCoordinates(
    ActorControlInputComponentState&, std::uint32_t actor,
    ActorControlVec3& output) {
    ActorControlMouseProjectionRuntimeStub runtime;
    return ActorControlMouseProjection::Project(actor, output, runtime);
}
float ActorControlFreeMovementRuntimeStub::ControllerThreshold() {
    return 0.0f;
}
double ActorControlFreeMovementRuntimeStub::ApproximateMagnitude(
    const ActorControlVec3&) { return 0.0; }
void ActorControlFreeMovementRuntimeStub::NormalizeDirection(
    ActorControlInputComponentState&) {}
void ActorControlFreeMovementRuntimeStub::DispatchVectorMovement(
    ActorControlInputComponentState&, std::uint32_t, float) {}

namespace {

float ReadFloat(IActorControlFreeMovementRuntime& runtime,
               std::uint32_t address) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

bool InputActive(IActorControlFreeMovementRuntime& runtime,
                 std::uint32_t count_offset, std::uint32_t flag_offset) {
    const auto manager = runtime.InputManager();
    const auto count = std::bit_cast<std::int32_t>(
        runtime.ReadInputWord(manager, count_offset));
    if (count <= 0) return false;
    return (runtime.ReadInputWord(manager, flag_offset) & 0xfu) != 0u;
}

ActorControlVec3 CameraBasis(IActorControlFreeMovementRuntime& runtime,
                             std::uint32_t row_offset) {
    const auto camera = runtime.ActiveCameraObjectPointer(
        runtime.CameraSystem());
    const auto object = runtime.ReadWord(camera + 4u);
    const auto transform = runtime.ObjectTransform(object);
    return {ReadFloat(runtime, transform + row_offset),
            ReadFloat(runtime, transform + row_offset + 4u),
            ReadFloat(runtime, transform + row_offset + 8u)};
}

ActorControlVec3 Direction(const ActorControlInputComponentState& state) {
    return {std::bit_cast<float>(state.words[6]),
            std::bit_cast<float>(state.words[7]),
            std::bit_cast<float>(state.words[8])};
}

void StoreDirection(ActorControlInputComponentState& state,
                    const ActorControlVec3& direction) {
    state.words[6] = std::bit_cast<std::uint32_t>(direction.x);
    state.words[7] = std::bit_cast<std::uint32_t>(direction.y);
    state.words[8] = std::bit_cast<std::uint32_t>(direction.z);
}

}  // namespace

std::uint32_t ActorControlInputComponent::UpdateFreeMovement(
    ActorControlInputComponentState& state, std::uint32_t actor,
    float delta_seconds, IActorControlFreeMovementRuntime& runtime) {
    state.words[4] = 0u;
    if (InputActive(runtime, 0x1f44u, 0x1f38u) ||
        InputActive(runtime, 0x1e34u, 0x1e28u)) {
        auto forward = CameraBasis(runtime, 0x20u);
        forward.y = 0.0f;
        StoreDirection(state, forward);
        state.words[4] = 1u;
    }
    if (InputActive(runtime, 0x1f54u, 0x1f48u) ||
        InputActive(runtime, 0x1df4u, 0x1de8u)) {
        auto backward = CameraBasis(runtime, 0x20u);
        backward.y = 0.0f;
        backward.x = -backward.x;
        backward.z = -backward.z;
        StoreDirection(state, backward);
        state.words[4] = 1u;
    }
    if (InputActive(runtime, 0x1f24u, 0x1f18u) ||
        InputActive(runtime, 0x1cd4u, 0x1cc8u)) {
        const auto right = CameraBasis(runtime, 0u);
        auto direction = Direction(state);
        direction.x += right.x;
        direction.y += right.y;
        direction.z += right.z;
        direction.y = 0.0f;
        StoreDirection(state, direction);
        state.words[4] = 1u;
    }
    if (InputActive(runtime, 0x1f34u, 0x1f28u) ||
        InputActive(runtime, 0x1d04u, 0x1cf8u)) {
        const auto right = CameraBasis(runtime, 0u);
        auto direction = Direction(state);
        direction.x -= right.x;
        direction.y -= right.y;
        direction.z -= right.z;
        direction.y = 0.0f;
        StoreDirection(state, direction);
        state.words[4] = 1u;
    }
    if (InputActive(runtime, 0x21d4u, 0x21c8u)) {
        ActorControlVec3 target{};
        if (runtime.MouseWorldCoordinates(state, actor, target)) {
            const auto delta_x = target.x - ReadFloat(runtime, actor + 0xacu);
            const auto actor_y = ReadFloat(runtime, actor + 0xb0u);
            const auto delta_y = actor_y - ReadFloat(runtime, actor + 0xb0u);
            const ActorControlVec3 delta{
                delta_x,
                delta_y,
                target.z - ReadFloat(runtime, actor + 0xb4u)};
            const auto threshold = runtime.ControllerThreshold();
            if (runtime.ApproximateMagnitude(delta) >
                static_cast<double>(threshold)) {
                StoreDirection(state, delta);
                state.words[7] = 0u;
                state.words[4] = 1u;
            }
        }
    }
    if (state.words[4] != 0u) {
        runtime.NormalizeDirection(state);
        runtime.DispatchVectorMovement(state, actor, delta_seconds);
        return state.words[4];
    }
    const auto result = state.words[4];
    state.words[8] = 0u;
    state.words[7] = 0u;
    state.words[6] = 0u;
    return result;
}

bool ActorControlInputComponent::HasMovementInput(
    const ActorControlInputComponentState& state,
    IActorControlFreeMovementRuntime& runtime) {
    return InputActive(runtime, 0x1f44u, 0x1f38u) ||
           InputActive(runtime, 0x1e34u, 0x1e28u) ||
           InputActive(runtime, 0x1f54u, 0x1f48u) ||
           InputActive(runtime, 0x1df4u, 0x1de8u) ||
           InputActive(runtime, 0x21d4u, 0x21c8u) ||
           state.words[9] != 0u;
}

}  // namespace pal4
