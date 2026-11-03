#include "pal4/actor_control_discrete_movement.h"
#include "pal4/actor_control_world_movement.h"
#include "pal4/actor_vector_movement.h"

#include <bit>
#include <cctype>
#include <string_view>

namespace pal4 {

std::uint32_t IActorControlDiscreteMovementRuntime::CameraActiveResult(
    std::uint32_t system) {
    return CameraActive(system) ? 1u : 0u;
}

std::uint32_t IActorControlDiscreteMovementRuntime::CanMoveResult(
    std::uint32_t adapter, ActorControlVec3& position,
    float lower_radius, float upper_radius) {
    return CanMove(adapter, position, lower_radius, upper_radius) ? 1u : 0u;
}

std::uint32_t IActorControlDiscreteMovementRuntime::UpdateFloorAttributesResult() {
    UpdateFloorAttributes();
    return 0u;
}

std::uint32_t ActorControlDiscreteMovementRuntimeStub::CameraSystem() {
    return 0u;
}
bool ActorControlDiscreteMovementRuntimeStub::CameraActive(std::uint32_t) {
    return false;
}
void ActorControlDiscreteMovementRuntimeStub::GetActiveCamera(std::uint32_t) {}
std::uint32_t ActorControlDiscreteMovementRuntimeStub::ReadWord(
    std::uint32_t) { return 0u; }
std::uint32_t ActorControlDiscreteMovementRuntimeStub::ObjectTransform(
    std::uint32_t) { return 0u; }
double ActorControlDiscreteMovementRuntimeStub::ActorProperty(
    std::uint32_t) { return 0.0; }
std::uint32_t ActorControlDiscreteMovementRuntimeStub::Controller() {
    return 0u;
}
void ActorControlDiscreteMovementRuntimeStub::Normalize(ActorControlVec3&) {}
void ActorControlDiscreteMovementRuntimeStub::SetAnimationSpeed(
    std::uint32_t, float) {}
std::string ActorControlDiscreteMovementRuntimeStub::AnimationName(
    std::uint32_t) { return {}; }
void ActorControlDiscreteMovementRuntimeStub::SetAnimationName(
    std::uint32_t, std::string_view) {}
void ActorControlDiscreteMovementRuntimeStub::SetAnimationFlag(
    std::uint32_t, std::uint32_t) {}
double ActorControlDiscreteMovementRuntimeStub::AngleFromDot(float) {
    return 0.0;
}
bool ActorControlDiscreteMovementRuntimeStub::AnyInputActive(
    const ActorControlInputComponentState&) { return false; }
std::uint32_t ActorControlDiscreteMovementRuntimeStub::MovementAdapter() {
    return 0u;
}
void ActorControlDiscreteMovementRuntimeStub::PreprocessMovement(
    std::uint32_t, ActorControlMovementGlobals&, ActorControlVec3&,
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}
void ActorControlDiscreteMovementRuntimeStub::UpdateMovement(
    std::uint32_t, ActorControlVec3&, float, float, std::uint32_t) {}
bool ActorControlDiscreteMovementRuntimeStub::CanMove(
    std::uint32_t, ActorControlVec3&, float, float) { return false; }
void ActorControlDiscreteMovementRuntimeStub::SetActorPosition(
    std::uint32_t, const ActorControlVec3&) {}
void ActorControlDiscreteMovementRuntimeStub::RotateActor(
    std::uint32_t, const ActorControlVec3&, std::uint32_t) {}
void ActorControlDiscreteMovementRuntimeStub::UpdateMinimap(
    const ActorControlVec3&, std::uint32_t) {}
void ActorControlDiscreteMovementRuntimeStub::UpdateFloorAttributes() {}

namespace {

float ReadFloat(IActorControlDiscreteMovementRuntime& runtime,
                std::uint32_t address) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

bool EqualAsciiIgnoringCase(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        const auto a = std::tolower(static_cast<unsigned char>(left[i]));
        const auto b = std::tolower(static_cast<unsigned char>(right[i]));
        if (a != b) return false;
    }
    return true;
}

void SelectAnimation(IActorControlDiscreteMovementRuntime& runtime,
                     std::uint32_t animation, std::string_view target) {
    if (!EqualAsciiIgnoringCase(target, runtime.AnimationName(animation))) {
        runtime.SetAnimationName(animation, target);
        runtime.SetAnimationFlag(animation, 1u);
    }
}

using MovementRadii=ActorMovementRadii;

MovementRadii PrepareMovementBounds(
    IActorControlDiscreteMovementRuntime& runtime,
    ActorControlMovementGlobals& globals, std::uint32_t actor,
    const ActorControlVec3& position) {
    const float lower_radius = static_cast<float>(
        runtime.ActorProperty(actor) *
        static_cast<double>(std::bit_cast<float>(0x3eb33333u)));
    const float upper_radius = static_cast<float>(
        runtime.ActorProperty(actor) *
        static_cast<double>(std::bit_cast<float>(0x3f07ae14u)));
    // IDA427784..4277D4: PC24 rounds the height products before adding Y.
    // A widened product/add shifts the sphere center and its wall correction.
    const float half_height = static_cast<float>(runtime.ActorProperty(actor) * 0.5);
    const float lower_height = half_height * 0.75f;
    const float upper_height = half_height * 1.25f;
    globals.lower = {
        position.x,
        lower_height + position.y,
        position.z};
    globals.upper = {
        position.x,
        upper_height + position.y,
        position.z};
    globals.upper_tag = runtime.ReadWord(runtime.Controller() + 0x24u);
    globals.lower_tag = runtime.ReadWord(runtime.Controller() + 0x24u);
    return {lower_radius, upper_radius};
}

std::uint32_t ApplyMouseRotation(
    const ActorControlInputComponentState& state,
    std::uint32_t actor, std::int32_t input,
    IActorControlDiscreteMovementRuntime& runtime) {
    const auto scale = std::bit_cast<float>(state.words[3]);
    const ActorControlVec3 rotation{
        0.0f,
        -static_cast<float>(static_cast<double>(input) *
                            static_cast<double>(scale)),
        0.0f};
    runtime.RotateActor(actor, rotation, 1u);
    const ActorControlVec3 position{
        ReadFloat(runtime, actor + 0xacu),
        ReadFloat(runtime, actor + 0xb0u),
        ReadFloat(runtime, actor + 0xb4u)};
    runtime.UpdateMinimap(position, actor + 0xa0u);
    return 1u;
}

class LegacyVectorMovement final : public IActorVectorMovementRuntime {
public:
    LegacyVectorMovement(ActorControlInputComponentState& state,ActorControlMovementGlobals& globals,
                         std::uint32_t actor,std::uint32_t input,IActorControlDiscreteMovementRuntime& runtime)
        : state_(state),globals_(globals),actor_(actor),input_(input),runtime_(runtime) {}
    void Begin() override { runtime_.GetActiveCamera(runtime_.CameraSystem()); }
    ActorControlVec3 Position() override {
        return {ReadFloat(runtime_,actor_+0xacu),ReadFloat(runtime_,actor_+0xb0u),ReadFloat(runtime_,actor_+0xb4u)};
    }
    float Speed() override { return std::bit_cast<float>(state_.words[1]); }
    ActorControlVec3 Forward() override {
        actor_object_=runtime_.ReadWord(actor_+0x14u);
        const auto model=runtime_.ReadWord(actor_object_+4u);
        const auto transform=runtime_.ObjectTransform(model);
        return {ReadFloat(runtime_,transform+0x20u),ReadFloat(runtime_,transform+0x24u),ReadFloat(runtime_,transform+0x28u)};
    }
    ActorMovementRadii PrepareBounds(ActorControlVec3 position) override {
        return PrepareMovementBounds(runtime_,globals_,actor_,position);
    }
    std::uint32_t Mode() override { return state_.words[10]; }
    void SelectMoveAnimation(std::string_view name) override { SelectAnimation(runtime_,actor_+0x170u,name); }
    void Preprocess(ActorControlVec3& displacement) override {
        runtime_.PreprocessMovement(runtime_.MovementAdapter(),globals_,displacement,actor_object_,1u,1u,1u);
    }
    void UpdateGround(ActorControlVec3& position,ActorMovementRadii radii) override {
        runtime_.UpdateMovement(runtime_.MovementAdapter(),position,radii.lower,radii.upper,1u);
    }
    bool CanMove(ActorControlVec3& position,ActorMovementRadii radii) override {
        return runtime_.CanMove(runtime_.MovementAdapter(),position,radii.lower,radii.upper);
    }
    void SetPosition(ActorControlVec3 position) override { runtime_.SetActorPosition(actor_,position); }
    void FaceInput() override {
        double angle=runtime_.AngleFromDot(ReadFloat(runtime_,input_+8u));
        if (ReadFloat(runtime_,input_)<0) angle=6.283185307179586-angle;
        const ActorControlVec3 facing{ReadFloat(runtime_,actor_+0xa0u),
            static_cast<float>(angle*static_cast<double>(std::bit_cast<float>(0x42652ee1u))),
            ReadFloat(runtime_,actor_+0xa8u)};
        runtime_.RotateActor(actor_,facing,0u);
    }
    void UpdateFloor() override { runtime_.UpdateFloorAttributes(); }
private:
    ActorControlInputComponentState& state_;
    ActorControlMovementGlobals& globals_;
    std::uint32_t actor_,input_,actor_object_{};
    IActorControlDiscreteMovementRuntime& runtime_;
};

}  // namespace

std::uint32_t ActorControlInputComponent::DispatchDiscreteMovement(
    ActorControlInputComponentState& state,
    ActorControlMovementGlobals& globals, std::uint32_t actor,
    std::uint32_t command, float delta_seconds,
    IActorControlDiscreteMovementRuntime& runtime) {
    const auto camera_system = runtime.CameraSystem();
    if (!runtime.CameraActive(camera_system) || actor == 0u)
        return 0u;
    runtime.GetActiveCamera(runtime.CameraSystem());

    const auto actor_object = runtime.ReadWord(actor + 0x14u);
    const auto model_object = runtime.ReadWord(actor_object + 4u);
    const auto transform = runtime.ObjectTransform(model_object);
    ActorControlVec3 direction{
        ReadFloat(runtime, transform + 0x20u),
        ReadFloat(runtime, transform + 0x24u),
        ReadFloat(runtime, transform + 0x28u)};
    ActorControlVec3 position{
        ReadFloat(runtime, actor + 0xacu),
        ReadFloat(runtime, actor + 0xb0u),
        ReadFloat(runtime, actor + 0xb4u)};
    const float movement = static_cast<float>(
        static_cast<double>(delta_seconds) *
        static_cast<double>(std::bit_cast<float>(state.words[1])));
    const float turning = static_cast<float>(
        static_cast<double>(delta_seconds) *
        static_cast<double>(std::bit_cast<float>(state.words[2])));
    const auto radii = PrepareMovementBounds(runtime, globals, actor,
                                             position);
    runtime.Normalize(direction);
    direction.y = 0.0f;

    const auto animation = actor + 0x170u;
    if (command == 1u) {
        std::string_view target;
        switch (state.words[10]) {
        case 0u:
            target = "C02";
            runtime.SetAnimationSpeed(animation, 1.0f);
            break;
        case 1u:
            target = "C03";
            runtime.SetAnimationSpeed(animation, 1.0f);
            break;
        case 2u:
            target = "C03";
            runtime.SetAnimationSpeed(animation,
                                      std::bit_cast<float>(0x3fb33333u));
            break;
        default:
            break;
        }
        SelectAnimation(runtime, animation, target);
    } else if (command == 2u) {
        SelectAnimation(runtime, animation, "C02");
        direction.x = -direction.x;
        direction.y = -direction.y;
        direction.z = -direction.z;
    } else if (command == 3u || command == 4u) {
        if (!runtime.AnyInputActive(state))
            SelectAnimation(runtime, animation, "C02");
        const ActorControlVec3 rotation{
            0.0f, command == 3u ? turning : -turning, 0.0f};
        runtime.RotateActor(actor, rotation, 1u);
        runtime.UpdateMinimap(position, actor + 0xa0u);
        return 1u;
    } else {
        return 0u;
    }

    const float distance = command == 2u ? movement * 0.5f : movement;
    ActorControlVec3 displacement{
        direction.x * distance,
        direction.y * distance,
        direction.z * distance};
    runtime.PreprocessMovement(runtime.MovementAdapter(), globals,
                               displacement, actor_object, 1u, 1u, 1u);
    position.x += displacement.x;
    position.y += displacement.y;
    position.z += displacement.z;
    runtime.UpdateMovement(runtime.MovementAdapter(), position,
                           radii.lower, radii.upper, 1u);
    if (runtime.CanMove(runtime.MovementAdapter(), position,
                        radii.lower, radii.upper)) {
        runtime.SetActorPosition(actor, position);
        runtime.UpdateMinimap(position, actor + 0xa0u);
        runtime.UpdateFloorAttributes();
    }
    return 1u;
}

std::uint32_t ActorControlInputComponent::ApplyMouseTurn(
    ActorControlInputComponentState& state, std::uint32_t actor,
    std::uint32_t input_value_address, std::uint32_t,
    IActorControlDiscreteMovementRuntime& runtime) {
    if (!runtime.AnyInputActive(state))
        SelectAnimation(runtime, actor + 0x170u, "C01");
    const auto input = std::bit_cast<std::int32_t>(
        runtime.ReadWord(input_value_address));
    return ApplyMouseRotation(state, actor, input, runtime);
}

std::uint32_t ActorControlInputComponent::ApplyMouseTurnValue(
    ActorControlInputComponentState& state, std::uint32_t actor,
    std::int32_t input_value,
    IActorControlDiscreteMovementRuntime& runtime) {
    if (!runtime.AnyInputActive(state))
        SelectAnimation(runtime, actor + 0x170u, "C01");
    return ApplyMouseRotation(state, actor, input_value, runtime);
}

std::uint32_t ActorControlInputComponent::DispatchVectorMovement(
    ActorControlInputComponentState& state,
    ActorControlMovementGlobals& globals, std::uint32_t actor,
    std::uint32_t direction_address, float delta_seconds,
    IActorControlDiscreteMovementRuntime& runtime) {
    LegacyVectorMovement adapter(state,globals,actor,direction_address,runtime);
    (void)DispatchActorVectorMovement(delta_seconds,adapter);
    return 1u;
}

std::uint32_t ActorControlWorldMovement::ApplyDelta(
    std::uint32_t actor, std::uint32_t delta_address,
    ActorControlMovementGlobals& globals,
    IActorControlDiscreteMovementRuntime& runtime) {
    const auto camera_result = runtime.CameraActiveResult(
        runtime.CameraSystem());
    if (camera_result == 0u || actor == 0u)
        return camera_result;
    runtime.GetActiveCamera(runtime.CameraSystem());
    ActorControlVec3 displacement{
        ReadFloat(runtime, delta_address),
        ReadFloat(runtime, delta_address + 4u),
        ReadFloat(runtime, delta_address + 8u)};
    ActorControlVec3 position{
        ReadFloat(runtime, actor + 0xacu),
        ReadFloat(runtime, actor + 0xb0u),
        ReadFloat(runtime, actor + 0xb4u)};
    const auto radii = PrepareMovementBounds(runtime, globals, actor,
                                             position);
    const auto actor_object = runtime.ReadWord(actor + 0x14u);
    runtime.PreprocessMovement(runtime.MovementAdapter(), globals,
                               displacement, actor_object, 1u, 1u, 1u);
    position.x += displacement.x;
    position.y += displacement.y;
    position.z += displacement.z;
    runtime.UpdateMovement(runtime.MovementAdapter(), position,
                           radii.lower, radii.upper, 1u);
    const auto collision_result = runtime.CanMoveResult(
        runtime.MovementAdapter(), position,
        radii.lower, radii.upper);
    if (collision_result == 0u) return collision_result;
    runtime.SetActorPosition(actor, position);
    runtime.UpdateMinimap(position, actor + 0xa0u);
    return runtime.UpdateFloorAttributesResult();
}

}  // namespace pal4
