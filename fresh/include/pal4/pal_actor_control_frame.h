#pragma once

#include "pal4/pal_actor_control_dispatch.h"
#include "pal4/pal_actor_control_yaw_input.h"
#include "pal4/actor_control_cursor_point.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorControlCameraAngles {
    float yaw = 0.0f;
    float pitch = 0.0f;
};

class IPalActorControlFrameRuntime : public IActorControlDispatchRuntime,
                                     public IActorControlYawRuntime {
public:
    virtual ~IPalActorControlFrameRuntime() = default;
    ActorControlMouseInput ReadYawInput() override;
    float ReadCameraYaw(std::uint32_t camera_internal) override;
    void ApplyCameraYaw(std::uint32_t camera_internal,
                        float yaw, bool update_matrix) override;
    virtual std::uint32_t ArenaMode();
    virtual bool ConsoleActive();
    virtual std::uint32_t ActiveCamera();
    virtual bool DialogNeedsUpdate(std::uint32_t* dialog);
    virtual void UpdateDialog(std::uint32_t* dialog);
    virtual void UpdateEffectProperties(std::uint32_t* dialog,
                                        std::uint32_t actor,
                                        bool& output);
    virtual void UpdateTimersAndUi(PalActorControlState& state, float delta);
    virtual bool HandleModeSwitch(PalActorControlState& state, float delta);
    virtual bool TimerFrameActive();
    virtual bool ScenarioDialogVisible();
    virtual float CameraFollowValue();
    virtual bool ArenaSceneReady();
    virtual std::uint32_t CameraInternal(std::uint32_t camera);
    virtual std::uint32_t HandleCameraYawInput(
        PalActorControlState& state, float delta,
        std::uint32_t camera_internal);
    virtual bool HandleCameraInput(PalActorControlState& state,
                                   float delta, std::uint32_t actor,
                                   std::uint32_t camera_internal);
    virtual void RestoreCamera(std::uint32_t camera,
                               float yaw, float pitch, bool look_at);
    virtual std::uint32_t UpdateYaw(PalActorControlState& state,
                                    std::uint32_t camera_internal,
                                    std::uint32_t actor, float step);
    virtual void FlushTailYaw(PalActorControlState& state);
    virtual bool AdvanceSpecial(std::uint32_t special,
                                std::uint32_t actor, float delta);
    virtual bool AdvanceComponent(std::uint32_t* component,
                                  std::uint32_t actor, float delta);
    virtual void BindSubobject(std::uint32_t* component,
                               std::uint32_t actor);
    virtual ActorControlMouseInput MouseInput();
    virtual ActorControlCursorPoint CursorPosition();
    virtual void QueryWindowRect();
    virtual ActorControlCameraAngles CameraAngles(std::uint32_t camera);
    virtual void SetCameraYaw(std::uint32_t camera, float yaw);
    virtual void SetCameraPitch(std::uint32_t camera, float pitch);
    virtual void AdjustCameraAngles(std::uint32_t camera,
                                    float yaw_delta, float pitch_delta);
    virtual bool SubobjectBusy(std::uint32_t* component);
    virtual void ReleaseDialog(PalActorControlState& state);
    virtual bool AnimationFlag(std::uint32_t actor);
    virtual std::string_view AnimationName(std::uint32_t actor);
    virtual std::uint32_t SetAnimation(std::uint32_t actor,
                                       std::string_view name);
    virtual void SetAnimationFlag(std::uint32_t actor,
                                  std::uint32_t value);
};

class PalActorControlFrame final {
public:
    // IDA 0x424750. Its sole caller ignores the machine-code return register.
    static void Update(PalActorControlState& state, float delta,
                       IPalActorControlFrameRuntime& runtime);
};

}  // namespace pal4
