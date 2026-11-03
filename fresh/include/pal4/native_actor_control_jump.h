#pragma once
#include "pal4/native_gob_machine_runtime.h"
#include <array>
#include <functional>
#include <memory>
namespace pal4 {
class NativeActorControlSession;
class InputManager;
class NativeCeguiUiSession;
class NativeGameArenaManagerOwner;
class NativeScenePlayerMovement;
class NativePlayerTransformRuntime;
class NativeScriptPlayerActionRuntime;
class NativeScriptPlayerPositionRuntime;
class NativeMinimapManagerOwner;
class NativeCameraManagerOwner;
class NativeEffectSceneSession;
struct NativeActorControlJumpServices {
    std::function<void(std::uint32_t)> ui_state;
    std::function<void()> stop_monsters;
    std::function<void(const std::array<float,3>&)> audio_position;
    std::function<void(std::string_view,std::uint32_t)> play_sound;
    std::function<std::string_view()> floor_sound;
    std::function<void(std::string_view,const std::array<float,3>&,std::uint32_t,float,float)> floor_audio;
    NativeGobMachineScriptServices script;
    std::function<std::string()> script_path;
};
// Borrows the live database at ActorControl+140. Physics is retained in its
// embedded ClumpVectorMotion words, never in a per-frame adapter copy.
class NativeActorControlJump final {
public:
    NativeActorControlJump(NativeActorControlSession&,InputManager&,NativeCeguiUiSession&,
        NativeGameArenaManagerOwner&,NativeScenePlayerMovement&,NativePlayerTransformRuntime&,
        NativeScriptPlayerActionRuntime&,NativeScriptPlayerPositionRuntime&,
        NativeMinimapManagerOwner&,NativeCameraManagerOwner&,NativeEffectSceneSession&,
        NativeActorControlJumpServices);
    ~NativeActorControlJump();
    bool ProcessDatabase(std::uint32_t* component,std::uint32_t actor,float elapsed);
    bool DatabaseHasPriority(std::uint32_t* component) const;
    bool DatabaseBusy(std::uint32_t* component) const;
    void BindComponent(std::uint32_t* component,std::uint32_t actor);
    std::uint32_t DatabaseAddress() const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
