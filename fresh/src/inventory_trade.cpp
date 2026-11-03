#include "pal4/inventory_trade.h"

#include <bit>

namespace pal4 {

std::int32_t InventoryTrade::Buy(
    const std::uint32_t item_id, IInventoryTradeRuntime& runtime) {
    const auto database = runtime.GameDatabaseInstance();
    const auto record = runtime.FindBuyRecord(database + kBuyTableOffset,
                                              item_id);
    if (record == 0u) return 0;

    const auto inventory = runtime.Inventory();
    const auto price = runtime.ReadWord(record + kPriceOffset);
    const auto balance = runtime.ReadWord(inventory + kBalanceOffset);
    if (std::bit_cast<std::int32_t>(balance) <
        std::bit_cast<std::int32_t>(price)) return 45;

    runtime.UpdateCounter(runtime.Inventory(), 0u - price);
    runtime.AddItem(runtime.Inventory() + kCollectionOffset, item_id);
    return 0;
}

std::uint8_t InventoryTrade::Sell(
    const std::uint32_t item_id, IInventoryTradeRuntime& runtime) {
    const auto inventory = runtime.Inventory();
    const auto record = runtime.FindSellRecord(
        inventory + kCollectionOffset, item_id, 7u);
    if (record == 0u) return 0u;

    const auto price = runtime.ReadWord(record + kPriceOffset);
    const auto proceeds = runtime.QuarterPrice(price);
    runtime.UpdateCounter(runtime.Inventory(), proceeds);
    return static_cast<std::uint8_t>(runtime.RemoveItem(
        runtime.Inventory() + kCollectionOffset, record));
}

}  // namespace pal4
