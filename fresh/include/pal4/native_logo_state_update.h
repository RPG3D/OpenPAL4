#pragma once
#include "pal4/logo_state.h"
#include "pal4/native_state_script_frame_services.h"

namespace pal4 {
class NativeLogoStateLifecycle;
class NativeUiFrameUpdate;
class NativeEffectSceneSession;
class NativeScriptFrameRunner;
class INativeScriptHandlerWaitRuntime;
struct PalScreenEffectState;
class IEffectRenderRuntime;
class IPreteritRuntime;

// Borrows all owners, including handler-only waits for this script context.
// Use OpeningWait.Handlers(), Flash.Handlers() or the timed-wait owner directly.
// The caller schedules this once per Logo frame; do not also tick those services.
// Rendering and state-registry dispatch remain separate capabilities.
class NativeLogoStateUpdate final : private ILogoStateUpdateRuntime {
public:
    NativeLogoStateUpdate(NativeLogoStateLifecycle&, NativeUiFrameUpdate&,
        NativeEffectSceneSession&, NativeScriptFrameRunner&,
        INativeScriptHandlerWaitRuntime&, PalScreenEffectState&,
        IEffectRenderRuntime&, IPreteritRuntime&);
    NativeLogoStateUpdate(NativeLogoStateLifecycle&, NativeUiFrameUpdate&,
        NativeEffectSceneSession&, NativeStateScriptFrameServices,
        INativeScriptHandlerWaitRuntime&, PalScreenEffectState&,
        IEffectRenderRuntime&, IPreteritRuntime&);
    std::uint32_t Update(float elapsed, bool suppress_screen_effect = false);
private:
    friend class NativeLogoStateSession;
    float ReadFloat(std::uint32_t) override;
    void WriteFloat(std::uint32_t, float) override;
    std::uint32_t UIFrameManager() override;
    std::uint32_t ScreenEffect() override;
    void ProcessScriptWaitHandlers(double) override;
    void AdvanceScreenEffect(std::uint32_t, float) override;
    void UpdateUI(std::uint32_t, double) override;
    std::uint32_t UpdateEffects(std::uint32_t, double) override;
    NativeLogoStateLifecycle& logo_;
    NativeUiFrameUpdate& ui_;
    NativeEffectSceneSession& effects_;
    NativeStateScriptFrameServices script_;
    INativeScriptHandlerWaitRuntime& waits_;
    PalScreenEffectState& screen_;
    IEffectRenderRuntime& render_;
    IPreteritRuntime& preterit_;
};
}
