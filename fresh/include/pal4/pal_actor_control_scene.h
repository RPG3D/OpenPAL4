#pragma once

#include "pal4/pal_actor_control_loop.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalActorControlSceneRuntime : public IPalActorControlLoopRuntime {
public:
    virtual std::string_view ActorName(std::uint32_t actor);
    virtual void InitializeCameraSubsystem();
    virtual std::uint32_t GetSceneCamera();
    virtual void SetSceneParameter(std::uint32_t camera,
                                   std::uint32_t actor_parameter_address);
    virtual void SetCameraLookAt(std::uint32_t camera, std::uint32_t enabled);
    virtual void AddMinimapIcon(std::string_view name,
                                std::string_view texture);
    virtual void SetActiveMinimapIcon(std::string_view name);
    virtual void FlushMainCameraTailYaw(PalActorControlState& state);
    virtual std::uint32_t GetCameraInternal(std::uint32_t camera);
    virtual float ComputeYawCorrection(PalActorControlState& state,
                                       std::uint32_t camera_internal,
                                       std::uint32_t actor,
                                       float& signed_component);
    virtual float CurrentCameraYaw(std::uint32_t camera_internal);
    virtual void SetCameraYaw(std::uint32_t camera_internal,
                              float yaw, std::uint32_t keep_look_at);
    virtual void BindSubobject(std::uint32_t* storage,
                               std::uint32_t actor,
                               std::uint32_t virtual_slot);
    virtual void AddOrReplaceAnimationMember(
        std::uint32_t animation_member, std::uint32_t* member);
    virtual void SetMonsterStateParameter(std::uint32_t actor);
    virtual std::uint32_t BottomHookAtomic(std::uint32_t actor,
                                           std::string_view name);
    virtual void SetModeParameters(std::uint32_t* component,
                                   std::uint32_t mode,
                                   std::uint32_t actor);
    virtual std::uint32_t FindMovementFrame();
    virtual void UpdateCharacterMovementState(std::uint32_t frame);
    void UpdateGameStateConditional(PalActorControlState& state,
                                    std::uint32_t value) override;
};

class PalActorControlScene final {
public:
    // IDA 0x423cc0, one actor-address argument; zero is a no-op.
    static void SetActor(PalActorControlState& state, std::uint32_t actor,
                         IPalActorControlSceneRuntime& runtime);
    static void FlushTailYaw(PalActorControlState& state,
                             IPalActorControlSceneRuntime& runtime);
    static void SetModeConditional(PalActorControlState& state,
                                   std::uint32_t mode,
                                   IPalActorControlSceneRuntime& runtime);
};

}  // namespace pal4
