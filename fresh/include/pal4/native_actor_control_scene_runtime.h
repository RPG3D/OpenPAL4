#pragma once

#include "pal4/actor_control_input_component.h"
#include "pal4/native_actor_camera_tail_yaw.h"
#include "pal4/pal_actor_control_flash_effect.h"
#include "pal4/pal_actor_control_scene.h"

#include <cstdint>
#include <functional>

namespace pal4 {

class BaseObjectState;
class NativeActorControlSession;
class NativeCameraManagerOwner;
class NativeMinimapManagerOwner;
class NativeMonsterManagerOwner;
class NativeActorAnimationStateBinding;
class NativeEffectSceneSession;

// Binds the Q01 player to the rewritten original SetActor call.
class NativeActorControlSceneRuntime final
    : public IPalActorControlSceneRuntime,
      private IActorControlInputComponentRuntime,
      private IActorControlFlashRuntime {
public:
    NativeActorControlSceneRuntime(
        NativeActorControlSession& control, NativeCameraManagerOwner& cameras,
        NativeMinimapManagerOwner& minimap,
        NativeMonsterManagerOwner& monsters,
        NativeActorAnimationStateBinding& animation,
        std::uint32_t arena);

    void SetActor(BaseObjectState& actor);
    void BindPreviousActor(BaseObjectState& actor) noexcept { previous_actor_=&actor; }
    void BindEffects(NativeEffectSceneSession& effects) noexcept { effects_=&effects; }
    // Change the already-selected actor's mode without repeating SetActor's
    // camera, minimap, animation membership and monster-target side effects.
    void SetMode(BaseObjectState& actor, std::uint32_t mode);
    void BindMovementFrame(std::uint32_t frame, std::function<void()> refresh);
    [[nodiscard]] const NativeActorCameraTailYawResult& TailYaw() const {
        return tail_yaw_;
    }

private:
    std::string_view ActorName(std::uint32_t actor) override;
    void InitializeCameraSubsystem() override;
    std::uint32_t GetSceneCamera() override;
    void SetSceneParameter(std::uint32_t camera,
                           std::uint32_t parameter) override;
    void SetCameraLookAt(std::uint32_t camera,
                         std::uint32_t enabled) override;
    void AddMinimapIcon(std::string_view name,
                        std::string_view texture) override;
    void SetActiveMinimapIcon(std::string_view name) override;
    void FlushMainCameraTailYaw(PalActorControlState& state) override;
    void BindSubobject(std::uint32_t* storage, std::uint32_t actor,
                       std::uint32_t virtual_slot) override;
    void AddOrReplaceAnimationMember(
        std::uint32_t animation_member, std::uint32_t* member) override;
    void SetModeParameters(std::uint32_t* component, std::uint32_t mode,
                           std::uint32_t actor) override;
    void UpdateGameStateAndUi(PalActorControlState& state,
                              std::uint32_t actor) override;
    void SetMonsterStateParameter(std::uint32_t actor) override;
    std::uint32_t BottomHookAtomic(std::uint32_t actor,
                                   std::string_view name) override;
    std::uint32_t GetPetManager() override;
    bool PetActive(std::uint32_t pet) override;
    std::uint32_t GetArenaManager() override;
    std::uint32_t FindMovementFrame() override;
    void UpdateCharacterMovementState(std::uint32_t frame) override;
    void SetActorAnimationFlag(std::uint32_t actor,
                               std::uint32_t value) override;

    std::int32_t GameConfigPrimary() override;
    void SetAnimationFlag(std::uint32_t member,
                          std::uint32_t value) override;
    void HandleInputEvents(ActorControlInputComponentState&,
                           std::uint32_t actor, float seconds) override;
    void UpdateInputObject(ActorControlInputComponentState&,
                           std::uint32_t actor, float seconds) override;
    bool NeedsUpdate(std::uint32_t* handle) override;
    void HandleEvent(std::uint32_t* handle,
                     std::uint32_t event) override;
    ActorControlEffectHandle CreateEffect(
        std::string_view name, std::uint32_t parameter,
        std::uint32_t enabled, std::uint32_t extra) override;
    std::uint32_t ReadEffectProperty(std::uint32_t actor) override;
    void UpdateProperties(std::uint32_t* handle,std::uint32_t frame) override;
    void RequireFlashHandle(const std::uint32_t*) const;

    NativeActorControlSession& control_;
    NativeCameraManagerOwner& cameras_;
    NativeMinimapManagerOwner& minimap_;
    NativeMonsterManagerOwner& monsters_;
    NativeActorAnimationStateBinding& animation_;
    std::uint32_t arena_;
    BaseObjectState* actor_ = nullptr;
    BaseObjectState* previous_actor_ = nullptr;
    NativeEffectSceneSession* effects_ = nullptr;
    std::uint32_t pending_parameter_ = 0u;
    NativeActorCameraTailYawResult tail_yaw_{};
    bool movement_frame_bound_ = false;
    std::uint32_t movement_frame_ = 0u;
    std::function<void()> refresh_movement_;
};

}  // namespace pal4
