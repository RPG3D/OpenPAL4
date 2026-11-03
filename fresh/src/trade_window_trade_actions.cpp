#include "pal4/trade_window_trade_actions.h"

#include <string>

namespace pal4 {
bool TradeWindowTradeActions::OnItemHover(
    const std::uint32_t frame, const std::uint32_t event,
    ITradeWindowConfirmationRuntime& runtime) {
    if (runtime.ReadWord(event + 0x1cu) == 0u) {
        const auto mode = runtime.ReadWord(frame + 0xe8u);
        if (mode == 0u || mode == 2u) {
            static_cast<void>(ShowTradePrompt(frame, runtime));
        }
    }
    return true;
}

bool TradeWindowTradeActions::OnItemTooltip(
    const std::uint32_t frame, ITradeWindowConfirmationRuntime& runtime) {
    static_cast<void>(ShowTradePrompt(frame, runtime));
    return true;
}


}  // namespace pal4
