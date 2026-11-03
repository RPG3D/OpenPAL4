#include "pal4/inventory_transactions.h"

#include <bit>

namespace pal4 {
namespace {

bool LessSigned(const std::uint32_t left, const std::uint32_t right) {
    return std::bit_cast<std::int32_t>(left) <
           std::bit_cast<std::int32_t>(right);
}

}  // namespace

std::int32_t InventoryTransactions::Add(
    const std::uint32_t item_id, const std::uint32_t quantity,
    IInventoryTransactionsRuntime& runtime) {
    const auto database = runtime.GameDatabaseInstance();
    const auto record = runtime.FindItemRecord(database, item_id);
    if (record == 0u) return 0;

    const auto total = quantity * runtime.ReadWord(record + kUnitPriceOffset);
    const auto inventory = runtime.Inventory();
    if (LessSigned(runtime.ReadWord(inventory + kBalanceOffset), total))
        return 45;

    runtime.UpdateCounter(runtime.Inventory(), 0u - total);
    runtime.ProcessItem(runtime.Inventory(), item_id, quantity);
    return 0;
}

std::int32_t InventoryTransactions::Remove(
    const std::uint32_t sequence, const std::uint32_t quantity,
    IInventoryTransactionsRuntime& runtime) {
    const auto inventory = runtime.Inventory();
    const auto entry = runtime.FindInventoryEntry(inventory, sequence, 5u);
    if (entry == 0u) return 0;
    if (LessSigned(runtime.ReadWord(entry + kEntryQuantityOffset), quantity))
        return 46;

    const auto item = runtime.ReadWord(entry);
    const auto price = std::bit_cast<std::int32_t>(
        runtime.ReadWord(item + kUnitPriceOffset));
    const auto unit_proceeds = std::bit_cast<std::uint32_t>(price / 4);
    runtime.UpdateCounter(runtime.Inventory(), quantity * unit_proceeds);
    const auto removal_item = runtime.ReadWord(entry);
    const auto removal_id = runtime.ReadWord(removal_item);
    runtime.RemoveInventoryItem(runtime.Inventory(), removal_id, quantity);
    return 0;
}

std::int32_t InventoryTransactions::AddMultiple(
    const std::span<const ItemQuantity> items,
    IInventoryTransactionsRuntime& runtime) {
    std::uint32_t total = 0u;
    for (const auto& item : items) {
        const auto database = runtime.GameDatabaseInstance();
        const auto record = runtime.FindItemRecord(database, item.id);
        if (record == 0u) return 0;
        total += runtime.ReadWord(record + kUnitPriceOffset) * item.quantity;
        const auto inventory = runtime.Inventory();
        if (LessSigned(runtime.ReadWord(inventory + kBalanceOffset), total))
            return 45;
    }
    for (const auto& item : items)
        static_cast<void>(Add(item.id, item.quantity, runtime));
    return 0;
}

std::int32_t InventoryTransactions::RemoveMultiple(
    const std::span<const ItemQuantity> items,
    IInventoryTransactionsRuntime& runtime) {
    for (const auto& item : items) {
        const auto inventory = runtime.Inventory();
        const auto entry = runtime.FindInventoryEntry(
            inventory, item.id, 5u);
        if (entry == 0u) return 0;
        if (LessSigned(runtime.ReadWord(entry + kEntryQuantityOffset),
                       item.quantity)) return 46;
    }
    for (const auto& item : items)
        static_cast<void>(Remove(item.id, item.quantity, runtime));
    return 0;
}

std::int32_t InventoryTransactions::LearnRecipe(
    const std::uint32_t recipe_token,
    IInventoryTransactionsRuntime& runtime) {
    const auto database = runtime.GameDatabaseInstance();
    const auto recipe = runtime.FindRecipeRecord(
        database + kRecipeTableOffset, recipe_token);
    if (recipe == 0u) return 0;

    const auto inventory = runtime.Inventory();
    if (runtime.HasRecipe(inventory + kRecipeCollectionOffset,
                          recipe_token)) return 0;

    const auto balance_owner = runtime.Inventory();
    const auto price = runtime.ReadWord(recipe + kRecipePriceOffset);
    if (LessSigned(runtime.ReadWord(balance_owner + kBalanceOffset), price))
        return 45;

    runtime.UpdateCounter(runtime.Inventory(), 0u - price);
    const auto learned_id = runtime.ReadWord(recipe + kLearnedIdOffset);
    runtime.LearnRecipe(runtime.Inventory() + kRecipeCollectionOffset,
                        learned_id, recipe_token);
    return 0;
}

}  // namespace pal4
