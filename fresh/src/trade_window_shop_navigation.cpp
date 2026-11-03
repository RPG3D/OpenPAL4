#include "pal4/trade_window_shop_navigation.h"

namespace pal4 {

void TradeWindowShopNavigation::UpdateShopData(
    const std::uint32_t frame, const std::uint32_t first_index,
    const bool forward, ITradeWindowShopNavigationRuntime& runtime) {
    auto index = first_index;
    const auto list = runtime.ReadWord(frame + 0x28u);
    for (;;) {
        if (runtime.ReadByte(frame + 0xf8u) != 0u) {
            runtime.ShowActivePage(frame, index);
        } else {
            runtime.ShowInactivePage(frame, index);
        }
        if (runtime.RowCount(list) != 0u) break;
        index = forward ? runtime.NextIndex(frame, index)
                        : runtime.PrevIndex(frame, index);
        if (index == first_index) {
            if (!runtime.HasTradeItems(frame)) {
                if (runtime.ReadByte(frame + 0xf8u) != 0u) {
                    runtime.ShowActivePage(frame, 0u);
                } else {
                    runtime.ShowInactivePage(frame, 0u);
                }
            }
            break;
        }
    }
    const auto visible = runtime.CanExecuteTrade(frame);
    runtime.SetVisible(runtime.ReadWord(frame + 0x44u), visible);
    runtime.SetVisible(runtime.ReadWord(frame + 0x48u), visible);
}

}  // namespace pal4
