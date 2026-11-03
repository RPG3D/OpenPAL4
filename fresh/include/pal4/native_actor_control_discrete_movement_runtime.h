#pragma once

#include "pal4/actor_control_discrete_movement.h"

namespace pal4 {

class NativeActorControlSession;
class NativeBaseObjectMemory;
class NativeCameraManagerOwner;
class NativeMinimapManagerOwner;
class NativePlayerTransformRuntime;
class NativeScriptPlayerActionRuntime;
class NativeScenePlayerMovement;
class NativeFloorAttributeOwner;
class InputManager;
class NativeScriptPlayerPositionRuntime;

// Connects discrete input to scene owners and the shared player position setter.
class NativeActorControlDiscreteMovementRuntime final
    : public ActorControlDiscreteMovementRuntimeStub {
public:
    NativeActorControlDiscreteMovementRuntime(
        NativeActorControlSession& control, NativeCameraManagerOwner& cameras,
        NativeMinimapManagerOwner& minimap,
        NativeScriptPlayerActionRuntime& actions,
        NativeScenePlayerMovement& movement,
        NativeFloorAttributeOwner& floor,
        NativePlayerTransformRuntime& transforms,
        NativeBaseObjectMemory& memory, InputManager& input,
        std::uint32_t actor, NativeScriptPlayerPositionRuntime& positions);

    std::uint32_t CameraSystem() override;
    bool CameraActive(std::uint32_t system) override;
    void GetActiveCamera(std::uint32_t system) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t ObjectTransform(std::uint32_t object) override;
    double ActorProperty(std::uint32_t actor) override;
    std::uint32_t Controller() override;
    void Normalize(ActorControlVec3& vector) override;
    void SetAnimationSpeed(std::uint32_t animation, float speed) override;
    std::string AnimationName(std::uint32_t animation) override;
    void SetAnimationName(std::uint32_t animation,
                          std::string_view name) override;
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t flag) override;
    double AngleFromDot(float value) override;
    bool AnyInputActive(const ActorControlInputComponentState& state) override;
    std::uint32_t MovementAdapter() override;
    void PreprocessMovement(std::uint32_t adapter,
                            ActorControlMovementGlobals& globals,
                            ActorControlVec3& displacement,
                            std::uint32_t actor_object,
                            std::uint32_t first, std::uint32_t second,
                            std::uint32_t third) override;
    void UpdateMovement(std::uint32_t adapter,
                        ActorControlVec3& position, float lower_radius,
                        float upper_radius, std::uint32_t enabled) override;
    bool CanMove(std::uint32_t adapter, ActorControlVec3& position,
                 float lower_radius, float upper_radius) override;
    void SetActorPosition(std::uint32_t actor,
                          const ActorControlVec3& position) override;
    void RotateActor(std::uint32_t actor,
                     const ActorControlVec3& rotation,
                     std::uint32_t enabled) override;
    void UpdateMinimap(const ActorControlVec3& position,
                       std::uint32_t actor_grid) override;
    void UpdateFloorAttributes() override;

private:
    void RequireActor(std::uint32_t actor) const;
    void RequireAnimation(std::uint32_t animation) const;

    NativeActorControlSession& control_;
    NativeCameraManagerOwner& cameras_;
    NativeMinimapManagerOwner& minimap_;
    NativeScriptPlayerActionRuntime& actions_;
    NativeScenePlayerMovement& movement_;
    NativeFloorAttributeOwner& floor_;
    NativePlayerTransformRuntime& transforms_;
    NativeBaseObjectMemory& memory_;
    InputManager& input_;
    std::uint32_t actor_;
    NativeScriptPlayerPositionRuntime& positions_;
};

}  // namespace pal4
