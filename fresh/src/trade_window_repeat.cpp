#include "pal4/trade_window.h"
#include "pal4/trade_window_trade_actions.h"
#include <bit>
namespace pal4 {
namespace {
void RemoveTradeEventBindings(std::uint32_t frame,ITradeWindowPressRuntime& runtime){
    runtime.RemoveObjectByIdentifier(runtime.ReadWord(runtime.FrameManager()+0x1a0u),frame+0xfcu);
    runtime.RemoveUiElementByIdentifier(runtime.ReadWord(runtime.FrameManager()+0x1a0u),frame+0x10cu);
}
bool RebindTradeList(const std::uint32_t frame, const std::uint32_t event,
                     const bool flag, ITradeWindowPressRuntime& runtime) {
    const auto value = runtime.ReadWord(event);
    RemoveTradeEventBindings(frame, runtime);
    const auto collection = runtime.ReadWord(runtime.FrameManager() + 0x1a0u);
    runtime.AddEventHandler(collection, frame + 0x10cu, 0x3e4ccccdu,
                            frame, 0x48cd80u, value, flag);
    return true;
}

}
bool TradeWindow::OnQuantityDecreaseDown(const std::uint32_t frame,
                                const std::uint32_t event,
                                ITradeWindowPressRuntime& runtime) {
    return RebindTradeList(frame, event, true, runtime);
}

bool TradeWindow::OnQuantityIncreaseDown(const std::uint32_t frame,
                            const std::uint32_t event,
                            ITradeWindowPressRuntime& runtime) {
    return RebindTradeList(frame, event, false, runtime);
}

bool TradeWindow::OnQuantityButtonUp(const std::uint32_t frame,
                                ITradeWindowPressRuntime& runtime) {
    RemoveTradeEventBindings(frame, runtime);
    return true;
}

void TradeWindowTradeActions::ModifyItemCount(
    const std::uint32_t frame, const std::int32_t row,
    const std::uint8_t decrement, ITradeWindowQuantityRuntime& runtime) {
    const auto list = runtime.ReadWord(frame + 0x28u);
    const auto item = runtime.GridItem(list, row, 1);
    const auto current = runtime.SpinnerValue(item);
    runtime.SetSpinnerValue(item, std::bit_cast<std::int32_t>(
        std::bit_cast<std::uint32_t>(current) + (decrement != 0u ? 0xffffffffu : 1u)));
    runtime.SetHoverRowIndex(list, row);
}

void TradeWindowTradeActions::OnListClick(
    const std::uint32_t frame, const std::int32_t row,
    const std::uint8_t decrement, ITradeWindowQuantityRuntime& runtime) {
    const auto collection = runtime.ReadWord(runtime.FrameManager() + 0x1a0u);
    runtime.RemoveObjectByIdentifier(collection, frame + 0xfcu);
    const auto target = runtime.ReadWord(runtime.FrameManager() + 0x1a0u);
    runtime.AddTimerHandler(target, frame + 0xfcu, 0x3dcccccdu,
                            frame, 0x48cd20u, row, decrement);
}


}
