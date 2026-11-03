#include "pal4/trade_window_trade_actions.h"
#include <string>
namespace pal4 {
void TradeWindowTradeActions::UpdateTotalPrice(
    const std::uint32_t frame, ITradeWindowTotalPriceRuntime& runtime) {
    if (runtime.ReadWord(frame + 0xe8u) != 1u) return;
    const auto active = runtime.ReadByte(frame + 0xf8u) != 0u;
    const auto list = runtime.ReadWord(frame + 0x28u);
    const auto rows = runtime.RowCount(list);
    // IDA reads an uninitialized stack word here. Use a defined initial sum.
    std::uint32_t total = 0u;
    for (std::int32_t row = 0; row < rows; ++row) {
        const auto item = runtime.GridItem(list, row, 1);
        const auto id = runtime.ReadWord(item + 0x138u);
        if (active) {
            if (!runtime.ContainsCharacterItem(runtime.GameDatabase(), id)) {
                continue;
            }
            const auto product = runtime.ResolveCharacterItem(
                runtime.GameDatabase(), id);
            const auto price = runtime.ReadWord(product + 0x90u);
            total += static_cast<std::uint32_t>(runtime.SpinnerValue(item)) *
                     price;
        } else {
            const auto entry = runtime.FindInventoryEntry(
                runtime.InventorySubsystem(), id, 5u);
            if (entry == 0u) continue;
            const auto product = runtime.ReadWord(entry);
            const auto price = static_cast<std::int32_t>(
                runtime.ReadWord(product + 0x90u)) / 4;
            total += static_cast<std::uint32_t>(runtime.SpinnerValue(item)) *
                     static_cast<std::uint32_t>(price);
        }
    }
    runtime.SetWindowText(runtime.ReadWord(frame + 0x38u),
                          std::to_string(static_cast<std::int32_t>(total)));
}

}
