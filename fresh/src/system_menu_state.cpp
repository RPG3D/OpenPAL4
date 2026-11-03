#include "pal4/system_menu_state.h"

namespace pal4 {

std::uint32_t SystemMenuState::EnterWrapper(
    std::uint32_t, std::uint32_t, ISystemMenuRuntime& runtime) {
    return Enter(runtime);
}

std::uint32_t SystemMenuState::Enter(ISystemMenuRuntime& runtime) {
    runtime.SetUIFlag(runtime.UIFrameManager(), 1u);
    runtime.WriteWord(runtime.Console(), 0u);
    runtime.AddUIElement(runtime.UIFrameManager(), "BJ");
    runtime.SetupUICamera(runtime.ReadWord(runtime.UIFrameManager() + 412u), 0u);
    runtime.CopyActiveCameraRaster(runtime.UIFrameManager());
    runtime.GetActiveCamera(runtime.ReadWord(runtime.UIFrameManager() + 412u));
    runtime.ActivateCamera(runtime.CameraManager(), runtime.UICameraName());
    runtime.DispatchEvent(runtime.UIFrameManager(), kFirstCloseEvent, 0u, 0u);
    runtime.DispatchEvent(runtime.UIFrameManager(), kSecondCloseEvent, 0u, 0u);
    runtime.ToggleWindows(runtime.UIFrameManager(), true);
    return 1u;
}

std::uint32_t SystemMenuState::Render(
    std::uint32_t frame, ISystemMenuRuntime& runtime) {
    runtime.PreRenderUI(runtime.UIFrameManager(), frame);
    return runtime.RenderUI(runtime.UIFrameManager(), frame);
}

std::uint32_t SystemMenuState::Update(
    float delta, std::uint32_t, std::uint32_t,
    ISystemMenuRuntime& runtime) {
    runtime.UpdateUI(runtime.UIFrameManager(), static_cast<double>(delta));
    return runtime.RenderEffectTransform(runtime.UIFrameManager(),
                                         static_cast<double>(delta));
}

std::uint32_t SystemMenuState::LeaveWrapper(
    std::uint32_t, ISystemMenuRuntime& runtime) {
    return Leave(runtime);
}

std::uint32_t SystemMenuState::Leave(ISystemMenuRuntime& runtime) {
    runtime.ToggleWindows(runtime.UIFrameManager(), false);
    runtime.CleanupEffects(runtime.UIFrameManager());
    return 1u;
}

}  // namespace pal4
