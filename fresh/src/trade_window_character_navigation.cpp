#include "pal4/trade_window_character_navigation.h"

namespace pal4 {

void TradeWindowCharacterNavigation::Update(
    const std::uint32_t frame, const std::uint32_t first_index,
    const bool forward, ITradeWindowCharacterNavigationRuntime& runtime) {
    const auto fill = [&runtime, frame](const std::uint32_t index) {
        if (runtime.ReadByte(frame + 0xf8u) != 0u)
            runtime.FillActiveCharacter(frame, index);
        else
            runtime.FillInactiveCharacter(frame, index);
    };

    auto index = first_index;
    for (;;) {
        fill(index);
        if (runtime.RowCount(runtime.ReadWord(frame + 0x28u)) != 0u) break;
        index = forward ? runtime.NextCharacterIndex(index)
                        : runtime.PreviousCharacterIndex(index);
        if (index == first_index) {
            if (!runtime.HasTradeItems(frame)) fill(0u);
            break;
        }
    }

    const auto visible = runtime.CanExecuteTrade(frame);
    runtime.SetVisible(runtime.ReadWord(frame + 0x44u), visible);
    runtime.SetVisible(runtime.ReadWord(frame + 0x48u), visible);
}

}  // namespace pal4
