#include "pal4/trade_state.h"

namespace pal4 {

std::uint32_t TradeState::EnterWrapper(
    std::uint32_t, std::uint32_t, ITradeStateRuntime& runtime) {
    return Enter(runtime);
}

std::uint32_t TradeState::Enter(ITradeStateRuntime& runtime) {
    runtime.SetUIFlag(runtime.UIFrameManager(), 1u);
    runtime.WriteWord(runtime.Console(), 0u);
    return 1u;
}

std::uint32_t TradeState::Render(
    std::uint32_t frame, ITradeStateRuntime& runtime) {
    if (runtime.ReadWord(runtime.TradeSystem()) != 0u) {
        runtime.PreRenderUI(runtime.UIFrameManager(), frame);
    } else {
        const auto value = runtime.FrameCamera(frame);
        runtime.ProcessWorldFrame(runtime.ArenaManager(), value);
    }
    return runtime.RenderUI(runtime.UIFrameManager(), frame);
}

std::uint32_t TradeState::Update(
    float delta, std::uint32_t, std::uint32_t,
    ITradeStateRuntime& runtime) {
    runtime.ProcessWaitHandlers(runtime.ScriptExecutor() + 84u,
                       static_cast<double>(delta));
    runtime.UpdateUI(runtime.UIFrameManager(), static_cast<double>(delta));
    if (runtime.ReadWord(runtime.TradeSystem()) == 0u)
        return runtime.ActivateCamera(runtime.CameraManager(),
                                      runtime.MainCameraName());
    static_cast<void>(runtime.ActivateCamera(runtime.CameraManager(),
                                             runtime.UICameraName()));
    return runtime.RenderEffectTransform(runtime.UIFrameManager(),
                                         static_cast<double>(delta));
}

std::uint32_t TradeState::LeaveWrapper(
    std::uint32_t, ITradeStateRuntime& runtime) {
    return Leave(runtime);
}

std::uint32_t TradeState::Leave(ITradeStateRuntime& runtime) {
    runtime.CloseShop(runtime.TradeSystem());
    runtime.CleanupEffects(runtime.UIFrameManager());
    return 1u;
}

}  // namespace pal4
