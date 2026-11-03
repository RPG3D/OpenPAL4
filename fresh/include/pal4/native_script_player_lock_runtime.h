#pragma once

#include "pal4/script_entity_callbacks.h"
#include "pal4/script_ui_state_callbacks.h"
#include "pal4/native_scene_ui_events.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class InputManager;
class NativeActorControlSession;
class NativeMonsterManagerOwner;
class NativeMinimapManagerOwner;
class NativeActorAnimationStateBinding;
class NativePlayerManagerSession;

class INativeScriptPlayerLockHost {
public:
    virtual ~INativeScriptPlayerLockHost() = default;
    virtual void SetDefaultCursor() = 0;
    // The host supplies the currently loaded arena's real return-path services.
    virtual void StopMonsterPursuit(NativeMonsterManagerOwner& monsters) = 0;
};

// Binds player lock/unlock and persistent ActorControl script flags to their
// live creating owner; UI events retain the existing state gate.
class NativeScriptPlayerLockRuntime final : public IScriptPlayerLockRuntime,
    public IScriptAlwaysJumpRuntime {
public:
    NativeScriptPlayerLockRuntime(
        NativePlayerManagerSession& players,
        NativeActorControlSession& control, InputManager& input,
        NativeMonsterManagerOwner& monsters,
        NativeSceneUiEvents events,
        INativeScriptPlayerLockHost& host);
    void BindAnimation(NativeActorAnimationStateBinding& animation) noexcept;
    void BindMinimap(NativeMinimapManagerOwner& minimap) noexcept;

    std::uint32_t PlayerManager() override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void SetActionCode(std::uint32_t component,
                       std::string_view code) override;
    std::uint32_t ActorControl() override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void ResetAndSync(std::uint32_t control, std::uint32_t flag) override;
    std::uint32_t UiFrameManager() override;
    std::uint8_t ProcessUiEvents(std::uint32_t manager,
                                 std::uint8_t flag) override;
    std::uint32_t MinimapManager() override;
    void UpdateGrid(std::uint32_t manager, std::uint32_t position,
                    std::uint32_t direction) override;

private:
    NativePlayerManagerSession& players_;
    NativeActorAnimationStateBinding* animation_ = nullptr;
    NativeMinimapManagerOwner* minimap_ = nullptr;
    NativeActorControlSession& control_;
    InputManager& input_;
    NativeMonsterManagerOwner& monsters_;
    NativeSceneUiEvents events_;
    INativeScriptPlayerLockHost& host_;
};

}  // namespace pal4
