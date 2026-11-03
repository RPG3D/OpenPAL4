#include "pal4/logo_state.h"

namespace pal4 {

std::uint32_t LogoState::Enter(
    std::uint32_t object, ILogoStateLifecycleRuntime& runtime) {
    runtime.SaveDialogSettings(runtime.GlobalData());
    runtime.SetUIFlag(runtime.UIFrameManager(), 1u);
    runtime.FadeInMusic(runtime.MusicPlayer(), "P01-1.mp3", 3.0, 0u);
    runtime.AddUIElement(runtime.UIFrameManager(), "ZJM");
    runtime.SetupUICamera(
        runtime.ReadWord(runtime.UIFrameManager() + 412u), 1u);
    runtime.SendUIEvent(runtime.UIFrameManager(), kOpenUIEvent, 1u, 0u);
    runtime.ActivateCamera(runtime.CameraManager(), runtime.UICameraName());
    runtime.ToggleWindows(runtime.UIFrameManager(), true);
    if (runtime.EffectNeedsUpdate(object + 4u))
        runtime.HandleEffect(object + 4u, 0u);

    constexpr std::array<float, 3> position{16.0f, 209.0f, -70.0f};
    const auto effect = runtime.CreateEffect(runtime.UIFrameManager(), "ZJM",
                                              position, 0xffffffffu, 0u);
    runtime.WriteWord(object + 4u, effect.first);
    runtime.WriteWord(object + 8u, effect.second);
    runtime.ResetTemporaryEffectHandle();
    constexpr std::array<float, 3> scale{1.2f, 1.2f, 1.2f};
    runtime.SetEffectProperties(object + 4u, scale, 0u);
    static_cast<void>(runtime.EffectNeedsUpdate(object + 4u));
    runtime.WriteWord(object + 12u, 0x40000000u);
    return 1u;
}

void LogoState::Render(std::uint32_t object, std::uint32_t frame,
                       ILogoStateRenderRuntime& runtime) {
    if (!runtime.IsBinkPlaying(runtime.BinkPlayer() + 4u))
        runtime.PreRenderUI(runtime.UIFrameManager(), frame);
    runtime.DrawScreenEffect(runtime.ScreenEffect(), frame);
    runtime.RenderBink(runtime.BinkPlayer(), frame);
    runtime.RenderUI(runtime.UIFrameManager(), frame);
    runtime.RenderConsole(runtime.Console(), frame);
    if (runtime.ReadFloat(object + 12u) <= 0.0f)
        runtime.DrawVersionText("PAL4 v1.1", 20,
                                static_cast<std::int32_t>(runtime.GameHeight()) - 80,
                                200, 20, 1u, 0xffefefefu, 16u);
}

std::uint32_t LogoState::Update(
    std::uint32_t object, float delta, std::uint32_t,
    std::uint32_t flag, ILogoStateUpdateRuntime& runtime) {
    const auto remaining = runtime.ReadFloat(object + 12u) - delta;
    runtime.WriteFloat(object + 12u,
                       remaining <= 0.0f ? 0.0f : remaining);
    runtime.ProcessScriptWaitHandlers(static_cast<double>(delta));
    if (flag == 0u)
        runtime.AdvanceScreenEffect(runtime.ScreenEffect(), delta);
    runtime.UpdateUI(runtime.UIFrameManager(), static_cast<double>(delta));
    return runtime.UpdateEffects(runtime.UIFrameManager(),
                                         static_cast<double>(delta));
}

std::uint32_t LogoState::LeaveWrapper(
    std::uint32_t object, std::uint32_t,
    ILogoStateLifecycleRuntime& runtime) {
    return Leave(object, runtime);
}

std::uint32_t LogoState::Leave(
    std::uint32_t object, ILogoStateLifecycleRuntime& runtime) {
    runtime.HandleEffect(object + 4u, 0u);
    runtime.SendUIEvent(runtime.UIFrameManager(), kCloseUIEvent, 0u, 0u);
    runtime.ToggleWindows(runtime.UIFrameManager(), false);
    runtime.CleanupEffects(runtime.UIFrameManager());
    return 1u;
}

}  // namespace pal4
