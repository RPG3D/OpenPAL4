#include "pal4/actor_control_idle.h"
#include "pal4/pal_actor_control_frame.h"
#include "pal4/pal_actor_control_movement.h"

#include <bit>
#include <cmath>
#include <initializer_list>

namespace pal4 {

ActorControlMouseInput IPalActorControlFrameRuntime::ReadYawInput() {
    return MouseInput();
}
float IPalActorControlFrameRuntime::ReadCameraYaw(
    std::uint32_t camera_internal) {
    return CameraAngles(camera_internal).yaw;
}
void IPalActorControlFrameRuntime::ApplyCameraYaw(
    std::uint32_t camera_internal, float yaw, bool) {
    SetCameraYaw(camera_internal, yaw);
}
std::uint32_t IPalActorControlFrameRuntime::ArenaMode() { return 0u; }
bool IPalActorControlFrameRuntime::ConsoleActive() { return false; }
std::uint32_t IPalActorControlFrameRuntime::ActiveCamera() { return 0u; }
bool IPalActorControlFrameRuntime::DialogNeedsUpdate(std::uint32_t*) {
    return false;
}
void IPalActorControlFrameRuntime::UpdateDialog(std::uint32_t*) {}
void IPalActorControlFrameRuntime::UpdateEffectProperties(
    std::uint32_t*, std::uint32_t, bool&) {}
void IPalActorControlFrameRuntime::UpdateTimersAndUi(
    PalActorControlState&, float) {}
bool IPalActorControlFrameRuntime::HandleModeSwitch(
    PalActorControlState&, float) { return false; }
bool IPalActorControlFrameRuntime::TimerFrameActive() { return false; }
bool IPalActorControlFrameRuntime::ScenarioDialogVisible() {
    return false;
}
float IPalActorControlFrameRuntime::CameraFollowValue() { return 0.0f; }
bool IPalActorControlFrameRuntime::ArenaSceneReady() { return false; }
std::uint32_t IPalActorControlFrameRuntime::CameraInternal(
    std::uint32_t) { return 0u; }
std::uint32_t IPalActorControlFrameRuntime::HandleCameraYawInput(
    PalActorControlState&, float delta, std::uint32_t camera_internal) {
    return PalActorControlYawInput::Handle(delta, camera_internal, *this);
}
bool IPalActorControlFrameRuntime::HandleCameraInput(
    PalActorControlState& state, float delta, std::uint32_t actor,
    std::uint32_t camera_internal) {
    return PalActorControlDispatch::Select(state, delta, actor,
                                            camera_internal, *this);
}
void IPalActorControlFrameRuntime::RestoreCamera(
    std::uint32_t, float, float, bool) {}
std::uint32_t IPalActorControlFrameRuntime::UpdateYaw(
    PalActorControlState& state, std::uint32_t camera_internal,
    std::uint32_t actor, float step) {
    IActorControlMovementRuntime runtime;
    return PalActorControlMovement::UpdateYaw(state, camera_internal,
                                              actor, step, runtime);
}
void IPalActorControlFrameRuntime::FlushTailYaw(
    PalActorControlState&) {}
bool IPalActorControlFrameRuntime::AdvanceSpecial(
    std::uint32_t, std::uint32_t, float) { return false; }
bool IPalActorControlFrameRuntime::AdvanceComponent(
    std::uint32_t*, std::uint32_t, float) { return false; }
void IPalActorControlFrameRuntime::BindSubobject(
    std::uint32_t*, std::uint32_t) {}
ActorControlMouseInput IPalActorControlFrameRuntime::MouseInput() {
    return {};
}
ActorControlCursorPoint IPalActorControlFrameRuntime::CursorPosition() {
    return {};
}
void IPalActorControlFrameRuntime::QueryWindowRect() {}
ActorControlCameraAngles IPalActorControlFrameRuntime::CameraAngles(
    std::uint32_t) { return {}; }
void IPalActorControlFrameRuntime::SetCameraYaw(
    std::uint32_t, float) {}
void IPalActorControlFrameRuntime::SetCameraPitch(
    std::uint32_t, float) {}
void IPalActorControlFrameRuntime::AdjustCameraAngles(
    std::uint32_t, float, float) {}
bool IPalActorControlFrameRuntime::SubobjectBusy(std::uint32_t*) {
    return false;
}
void IPalActorControlFrameRuntime::ReleaseDialog(
    PalActorControlState&) {}
bool IPalActorControlFrameRuntime::AnimationFlag(std::uint32_t) {
    return false;
}
std::string_view IPalActorControlFrameRuntime::AnimationName(
    std::uint32_t) { return {}; }
std::uint32_t IPalActorControlFrameRuntime::SetAnimation(
    std::uint32_t, std::string_view) { return 0u; }
void IPalActorControlFrameRuntime::SetAnimationFlag(
    std::uint32_t, std::uint32_t) {}

namespace {

float ReadFloat(const PalActorControlState& state, std::uint32_t index) {
    return std::bit_cast<float>(state.words[index]);
}

void WriteFloat(PalActorControlState& state, std::uint32_t index,
                float value) {
    state.words[index] = std::bit_cast<std::uint32_t>(value);
}


void RestoreCamera(PalActorControlState& state, std::uint32_t camera,
                   bool look_at, IPalActorControlFrameRuntime& runtime) {
    if (state.words[14] == 0u) return;
    runtime.RestoreCamera(camera, ReadFloat(state, 16u),
                          ReadFloat(state, 15u), look_at);
    for (const auto index : {14u, 15u, 16u, 17u, 18u})
        state.words[index] = 0u;
}

void AdvanceFollowYaw(PalActorControlState& state, std::uint32_t camera,
                      float delta, float follow_value,
                      bool allow_recenter,
                      IPalActorControlFrameRuntime& runtime) {
    if (follow_value == 0.0f) return;
    if (state.words[3] != 0u) {
        state.words[3] = runtime.UpdateYaw(
            state, runtime.CameraInternal(camera), state.words[1],
            delta * ReadFloat(state, 2u));
    }
    if (allow_recenter && state.words[3] == 0u)
        runtime.FlushTailYaw(state);
}

void UpdateMouseDrag(PalActorControlState& state, std::uint32_t camera,
                     IPalActorControlFrameRuntime& runtime) {
    const auto cursor = runtime.CursorPosition();
    runtime.QueryWindowRect();
    auto& words = state.words;
    const auto dx = cursor.x - static_cast<std::int32_t>(words[10]);
    const auto dy = cursor.y - static_cast<std::int32_t>(words[11]);
    words[10] = static_cast<std::uint32_t>(dx);
    words[11] = static_cast<std::uint32_t>(dy);
    if (words[14] == 0u) {
        words[14] = 1u;
        const auto angles = runtime.CameraAngles(camera);
        WriteFloat(state, 16u, angles.yaw);
        WriteFloat(state, 15u, angles.pitch);
        words[17] = 0u;
        words[18] = 0u;
    } else {
        const auto yaw_delta = -static_cast<float>(dx) *
                               ReadFloat(state, 13u);
        const auto pitch_delta = static_cast<float>(dy) *
                                 ReadFloat(state, 12u);
        const auto yaw_offset = ReadFloat(state, 18u) + yaw_delta;
        const auto pitch_offset = ReadFloat(state, 17u) + pitch_delta;
        WriteFloat(state, 18u, yaw_offset);
        WriteFloat(state, 17u, pitch_offset);
        if (yaw_offset < -60.0f) {
            WriteFloat(state, 18u, -60.0f);
            runtime.SetCameraYaw(camera, ReadFloat(state, 16u) - 60.0f);
        } else if (yaw_offset > 60.0f) {
            WriteFloat(state, 18u, 60.0f);
            runtime.SetCameraYaw(camera, ReadFloat(state, 16u) + 60.0f);
        } else {
            runtime.AdjustCameraAngles(camera, yaw_delta, 0.0f);
        }
        if (pitch_offset < -20.0f) {
            WriteFloat(state, 17u, -20.0f);
            runtime.SetCameraPitch(camera, ReadFloat(state, 15u) - 20.0f);
        } else if (pitch_offset > 20.0f) {
            WriteFloat(state, 17u, 20.0f);
            runtime.SetCameraPitch(camera, ReadFloat(state, 15u) + 20.0f);
        } else {
            runtime.AdjustCameraAngles(camera, 0.0f, pitch_delta);
        }
    }
    words[10] = static_cast<std::uint32_t>(cursor.x);
    words[11] = static_cast<std::uint32_t>(cursor.y);
}

void UpdateIdleAnimation(PalActorControlState& state,float delta,IPalActorControlFrameRuntime& runtime) {
    auto& words=state.words;
    const bool busy=words[183]!=0u || runtime.SubobjectBusy(words.data()+23u) ||
        runtime.SubobjectBusy(words.data()+35u) || runtime.SubobjectBusy(words.data()+165u);
    struct Adapter {
        PalActorControlState& state;IPalActorControlFrameRuntime& runtime;ActorControlIdleState& idle;
        void Sync(){WriteFloat(state,7u,idle.elapsed);state.words[8]=idle.action;}
        void ReleaseDialog(){Sync();runtime.ReleaseDialog(state);}
        bool Completed(){return runtime.AnimationFlag(state.words[1]);}
        std::string_view AnimationName(){return runtime.AnimationName(state.words[1]);}
        std::uint32_t SetAnimation(std::string_view name){return runtime.SetAnimation(state.words[1],name);}
        void SetRestart(){Sync();runtime.SetAnimationFlag(state.words[1],1);}
    };
    ActorControlIdleState idle{ReadFloat(state,7u),words[8]};
    Adapter adapter{state,runtime,idle};
    UpdateActorControlIdle(idle,delta,busy,adapter);
    WriteFloat(state,7u,idle.elapsed);words[8]=idle.action;
}

}  // namespace

void PalActorControlFrame::Update(PalActorControlState& state, float delta,
                                  IPalActorControlFrameRuntime& runtime) {
    auto& words = state.words;
    if (words[1] == 0u || words[0] == 0u ||
        runtime.ArenaMode() != 1u || runtime.ConsoleActive())
        return;
    const auto camera = runtime.ActiveCamera();
    if (camera == 0u) return;

    if (words[184] != 0u) {
        const auto remaining = ReadFloat(state, 185u) - delta;
        WriteFloat(state, 185u, remaining);
        if (remaining > 0.0f || std::isnan(remaining)) {
            if (runtime.DialogNeedsUpdate(words.data() + 186u)) {
                bool output = false;
                runtime.UpdateEffectProperties(words.data() + 186u,
                                               words[1], output);
            }
        } else {
            words[185] = 0u;
            words[184] = 0u;
            if (runtime.DialogNeedsUpdate(words.data() + 186u))
                runtime.UpdateDialog(words.data() + 186u);
        }
    }
    runtime.UpdateTimersAndUi(state, delta);
    if (runtime.HandleModeSwitch(state, delta) ||
        runtime.TimerFrameActive() || runtime.ScenarioDialogVisible())
        return;

    const auto follow_value = runtime.CameraFollowValue();
    bool allow_recenter = true;
    if (runtime.ArenaSceneReady()) {
        const auto input = runtime.HandleCameraYawInput(
            state, delta, runtime.CameraInternal(camera));
        if (input == 1u) {
            words[3] = 0u;
            allow_recenter = false;
        } else if (input == 2u) {
            words[3] = 1u;
            words[5] = 0u;
            allow_recenter = false;
        }
    }
    if (allow_recenter && words[4] != 0u) {
        words[3] = 1u;
        words[5] = 0u;
        allow_recenter = false;
    }

    const auto camera_internal = runtime.CameraInternal(camera);
    if (runtime.HandleCameraInput(state, delta, words[1],
                                  camera_internal)) {
        RestoreCamera(state, camera, false, runtime);
        if (words[183] != 0u) {
            AdvanceFollowYaw(state, camera, delta, follow_value,
                             allow_recenter, runtime);
            if (!runtime.AdvanceSpecial(words[183], words[1], delta))
                words[183] = 0u;
            words[4] = 0u;
        }
    } else if (runtime.AdvanceComponent(words.data() + 23u,
                                        words[1], delta)) {
        RestoreCamera(state, camera, true, runtime);
        AdvanceFollowYaw(state, camera, delta, follow_value,
                         allow_recenter, runtime);
        runtime.BindSubobject(words.data() + 35u, words[1]);
        runtime.BindSubobject(words.data() + 165u, words[1]);
        words[4] = 0u;
    } else {
        words[4] = 1u;
        if (runtime.ArenaSceneReady()) {
            const auto input = runtime.MouseInput();
            const bool primary_drag = input.primary_count > 0 &&
                                      (input.primary_mode & 0xfu) != 0u;
            const bool secondary_drag = input.secondary_count > 0 &&
                                        (input.secondary_mode & 0xfu) != 0u;
            if (primary_drag || secondary_drag) {
                UpdateMouseDrag(state, camera, runtime);
            } else if ((input.primary_count == 0 &&
                        input.primary_release != 0u) ||
                       (input.secondary_count == 0 &&
                        input.secondary_release != 0u)) {
                RestoreCamera(state, camera, true, runtime);
            }
        }
    }
    UpdateIdleAnimation(state, delta, runtime);
}

}  // namespace pal4
