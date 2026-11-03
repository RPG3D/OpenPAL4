#include "pal4/inventory_item_queries.h"

namespace pal4 {

std::uint32_t InventoryItemQueries::CopyItemData(
    const std::uint32_t destination, const std::uint32_t key,
    IInventoryItemQueryRuntime& runtime) {
    const auto inventory = runtime.InventorySubsystem();
    const auto entry = runtime.FindInventoryEntry(inventory + 0x7cu, key, 7u);
    UiTextBufferRecord::InitializeTextOnly(destination, entry + 0x98u,
                                           runtime);
    return destination;
}

std::uint32_t InventoryItemQueries::GetProperty(
    const std::uint32_t key, IInventoryItemQueryRuntime& runtime) {
    const auto inventory = runtime.InventorySubsystem();
    const auto entry = runtime.FindInventoryEntry(inventory + 0x7cu, key, 7u);
    return runtime.ReadWord(entry + 0x184u);
}

std::uint32_t InventoryItemQueries::FindGameDbValue(
    const std::uint32_t key, IInventoryItemQueryRuntime& runtime) {
    const auto inventory = runtime.InventorySubsystem();
    const auto entry = runtime.FindGameDbEntry(inventory, key, 5u);
    return entry == 0u ? 0u : runtime.ReadWord(entry + 8u);
}

std::int32_t InventoryItemQueries::CountMatching(
    const std::uint32_t key, IInventoryItemQueryRuntime& runtime) {
    const auto game_db = runtime.GameDb();
    const auto category = runtime.FindCategory(game_db + 0x58u, key);
    if (category == 0u) return 0;

    const auto inventory = runtime.InventorySubsystem();
    const auto count = runtime.ElementCount(
        inventory + 0x7cu, runtime.ReadWord(category + 0x94u));
    std::int32_t matches = 0;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto current_inventory = runtime.InventorySubsystem();
        const auto entry = runtime.ElementAt(
            current_inventory + 0x7cu,
            runtime.ReadWord(category + 0x94u), index);
        if (entry != 0u && runtime.ReadWord(entry + 0x90u) == key)
            ++matches;
    }
    return matches;
}

bool InventoryItemQueries::HasQuantityLimit(
    const std::uint32_t key, IInventoryItemQueryRuntime& runtime) {
    return CountMatching(key, runtime) >= 99;
}

bool InventoryItemQueries::IsBelowRecipeQuantity(
    const std::uint32_t recipe, const std::uint32_t item,
    IInventoryItemQueryRuntime& runtime) {
    const auto inventory = runtime.InventorySubsystem();
    const auto entry = runtime.FindInventoryEntry(inventory + 0x7cu, item, 7u);
    const auto db = runtime.GameDb();
    const auto record = runtime.FindRecipe(db + 0x80u, recipe);
    const auto difference = runtime.ReadWord(entry + 0x184u) -
                            runtime.ReadWord(record + 0x6cu);
    return (difference & 0x80000000u) != 0u;
}

bool InventoryItemQueries::MeetsRecipeProperty(
    const std::uint32_t recipe, const std::uint32_t item,
    IInventoryItemQueryRuntime& runtime) {
    const auto inventory = runtime.InventorySubsystem();
    const auto entry = runtime.FindInventoryEntry(inventory + 0x7cu, item, 7u);
    const auto db = runtime.GameDb();
    const auto record = runtime.FindRecipe(db + 0x80u, recipe);
    return static_cast<std::int32_t>(runtime.ReadWord(entry + 0x17cu)) >=
           static_cast<std::int32_t>(runtime.ReadWord(record + 0x70u));
}

bool InventoryItemQueries::HasItemProperty(
    const std::uint32_t item, IInventoryItemQueryRuntime& runtime) {
    const auto inventory = runtime.InventorySubsystem();
    const auto entry = runtime.FindInventoryEntry(inventory + 0x7cu, item, 7u);
    return runtime.ReadWord(entry + 0x180u) != 0u;
}

bool InventoryItemQueries::MatchesRecipeProperty(
    const std::uint32_t item, const std::uint32_t recipe,
    IInventoryItemQueryRuntime& runtime) {
    const auto inventory = runtime.InventorySubsystem();
    const auto entry = runtime.FindInventoryEntry(inventory + 0x7cu, item, 7u);
    const auto property = runtime.ReadWord(entry + 0x180u);
    if (property == 0u) return false;
    const auto db = runtime.GameDb();
    const auto record = runtime.FindRecipe(db + 0x80u, recipe);
    return runtime.ReadWord(record + 0x74u) == property;
}

}  // namespace pal4
