#include "pal4/trade_window.h"

namespace pal4 {

void TradeWindow::SetVisible(const std::uint32_t frame, const bool visible,
                             const std::uint32_t parent,
                             ITradeWindowLifecycleRuntime& runtime) {
    runtime.SetParent(frame, visible, parent);
    constexpr std::array<float, 3> effect_position{-59.669998f, 220.86f,
                                                    298.26001f};
    constexpr std::array<float, 3> effect_scale{2.2f, 2.2f, 2.2f};
    constexpr std::array<float, 3> flower_position{-59.669998f, 329.85999f,
                                                    298.26001f};
    constexpr std::array<float, 3> flower_scale{3.5f, 3.5f, 3.5f};
    if (visible) {
        const auto [first, second] = runtime.CreateEffect(
            runtime.FrameManager(), "LH", effect_position, 0xffffffffu, false);
        runtime.WriteWord(frame + 0x128u, first);
        runtime.WriteWord(frame + 0x12cu, second);
        runtime.ResetTemporaryEffect(first, second);
        runtime.SetEffectProperties(frame + 0x128u, effect_scale, false);
        runtime.MarkEffectForUpdate(frame + 0x128u);
        runtime.AddAndProcessScript();
        const auto collection = runtime.ReadWord(runtime.FrameManager() + 0x19cu);
        const auto object = runtime.CreateGameObject(
            collection, "ui\\model\\", "_ui_flower", true);
        if (object != 0u) {
            runtime.WriteWord(frame + 0x120u,
                              runtime.ReadWord(object + 0x170u));
            runtime.SetGameObjectScale(object, flower_scale);
            runtime.SetGameObjectPosition(object, flower_position);
        }
        runtime.CleanupScripts();
        return;
    }

    const auto collection = runtime.ReadWord(runtime.FrameManager() + 0x19cu);
    runtime.SetGameObjectDisplay(frame, 0u, 0u, 0u);
    runtime.RemoveGameObject(collection, runtime.ReadWord(frame + 0x120u));
    runtime.RemoveGameObject(collection, runtime.ReadWord(frame + 0x124u));
    runtime.WriteWord(frame + 0x120u, 0u);
    runtime.WriteWord(frame + 0x124u, 0u);
    runtime.ResetList(runtime.ReadWord(frame + 0x28u));
    runtime.SetWindowText(runtime.ReadWord(frame + 0x38u), "");
    runtime.HandleEffectEvent(frame + 0x128u, 0u);
}

bool TradeWindow::OnCloseThunk(const std::uint32_t frame,
                               ITradeWindowLifecycleRuntime& runtime) {
    return OnClose(frame, runtime);
}

bool TradeWindow::OnClose(const std::uint32_t frame,
                          ITradeWindowLifecycleRuntime& runtime) {
    runtime.RemoveActiveFrame(runtime.FrameManager() + 0x134u,
                              frame + 0x19cu);
    runtime.RemoveActiveFrame(runtime.FrameManager() + 0x134u,
                              frame + 0x1a8u);
    runtime.ResetTradeSystem(runtime.TradeSystem());
    runtime.ProcessUiEvent(runtime.FrameManager() + 0x104u,
                           0x8b0980u, 1u, 0u);
    return true;
}

bool TradeWindow::GlobalUpdateHandler(ITradeWindowLifecycleRuntime& runtime) {
    runtime.ProcessUiEvent(runtime.FrameManager() + 0x104u,
                           0x8b1c30u, 1u, 4u);
    return true;
}

}  // namespace pal4
