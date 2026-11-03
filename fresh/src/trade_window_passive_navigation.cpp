#include "pal4/trade_window_passive_navigation.h"

namespace pal4 {

void TradeWindowPassiveNavigation::Update(
    std::uint32_t frame, std::uint32_t first_index,
    bool forward, ITradeWindowPassiveNavigationRuntime& runtime) {
    auto index = first_index;
    for (;;) {
        if (runtime.ReadByte(frame + 0xf8u) != 0u)
            runtime.ShowActivePage(frame, index);
        if (runtime.RowCount(runtime.ReadWord(frame + 0x28u)) != 0u) break;
        index = forward ? runtime.NextIndex(frame, index)
                        : runtime.PreviousIndex(frame, index);
        if (index == first_index) {
            if (!runtime.HasTradeItems(frame))
                runtime.ShowActivePage(frame, 0u);
            break;
        }
    }
    const auto visible = runtime.CanExecuteTrade(frame);
    runtime.SetVisible(runtime.ReadWord(frame + 0x44u), visible);
    runtime.SetVisible(runtime.ReadWord(frame + 0x48u), visible);
}

}  // namespace pal4
