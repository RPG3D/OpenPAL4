#include "pal4/trade_window_trade_actions.h"
namespace pal4 {
bool TradeWindowTradeActions::OnHoverChanged(
    const std::uint32_t frame, ITradeWindowHoverRuntime& runtime) {
    const auto mode = runtime.ReadWord(frame + 0xe8u);
    const auto active = runtime.ReadByte(frame + 0xf8u) != 0u;
    if (active) {
        if (mode == 0u) {
            runtime.UpdateItemDisplay(frame, runtime.GetItemFromHover(frame));
        } else if (mode == 1u) {
            runtime.InvokeHoverHandler(0x496580u, frame,
                runtime.GetBuyItemFromHover(frame));
        } else if (mode == 2u) {
            runtime.InvokeHoverHandler(0x493910u, frame,
                runtime.GetPassiveFromHover(frame));
        }
    } else if (mode == 0u) {
        runtime.InvokeHoverHandler(0x490000u, frame,
            runtime.GetInventoryFromHover(frame));
    } else if (mode == 1u) {
        runtime.InvokeHoverHandler(0x496200u, frame,
            runtime.GetBuyInventoryFromHover(frame));
    }
    runtime.RefreshTotalPrice(frame);
    return true;
}

}
