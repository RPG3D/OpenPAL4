#pragma once

#include <cstdint>
#include <span>

namespace pal4 {

// Buy batches use definition IDs; sell batches use inventory sequence keys.
struct ItemQuantity {
    std::uint32_t id = 0u;
    std::uint32_t quantity = 0u;
};

class IInventoryTransactionsRuntime {
public:
    virtual ~IInventoryTransactionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDatabaseInstance() = 0;
    [[nodiscard]] virtual std::uint32_t FindItemRecord(
        std::uint32_t database, std::uint32_t item_id) = 0;
    [[nodiscard]] virtual std::uint32_t FindRecipeRecord(
        std::uint32_t table, std::uint32_t recipe_token) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void UpdateCounter(std::uint32_t inventory,
                               std::uint32_t signed_delta_bits) = 0;
    virtual void ProcessItem(std::uint32_t inventory,
                             std::uint32_t item_id,
                             std::uint32_t quantity) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryEntry(
        std::uint32_t inventory, std::uint32_t sequence,
        std::uint32_t category) = 0;
    virtual void RemoveInventoryItem(std::uint32_t inventory,
                                     std::uint32_t item_id,
                                     std::uint32_t quantity) = 0;
    [[nodiscard]] virtual bool HasRecipe(std::uint32_t collection,
                                          std::uint32_t recipe_token) = 0;
    virtual void LearnRecipe(std::uint32_t collection,
                             std::uint32_t learned_id,
                             std::uint32_t recipe_token) = 0;
};

class InventoryTransactions final {
public:
    static constexpr std::uint32_t kBalanceOffset = 308u;
    static constexpr std::uint32_t kUnitPriceOffset = 144u;
    static constexpr std::uint32_t kEntryQuantityOffset = 8u;
    static constexpr std::uint32_t kRecipeTableOffset = 128u;
    static constexpr std::uint32_t kRecipeCollectionOffset = 260u;
    static constexpr std::uint32_t kRecipePriceOffset = 120u;
    static constexpr std::uint32_t kLearnedIdOffset = 100u;

    // IDA 0x50ffa0 and 0x510010.
    [[nodiscard]] static std::int32_t Add(
        std::uint32_t item_id, std::uint32_t quantity,
        IInventoryTransactionsRuntime& runtime);
    [[nodiscard]] static std::int32_t Remove(
        std::uint32_t sequence, std::uint32_t quantity,
        IInventoryTransactionsRuntime& runtime);

    // IDA 0x510080 and 0x510140.
    [[nodiscard]] static std::int32_t AddMultiple(
        std::span<const ItemQuantity> items,
        IInventoryTransactionsRuntime& runtime);
    [[nodiscard]] static std::int32_t RemoveMultiple(
        std::span<const ItemQuantity> items,
        IInventoryTransactionsRuntime& runtime);

    // IDA 0x5101e0.
    [[nodiscard]] static std::int32_t LearnRecipe(
        std::uint32_t recipe_token,
        IInventoryTransactionsRuntime& runtime);
};

}  // namespace pal4
