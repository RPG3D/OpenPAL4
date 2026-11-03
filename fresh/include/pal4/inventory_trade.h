#pragma once

#include <cstdint>

namespace pal4 {

class IInventoryTradeRuntime {
public:
    virtual ~IInventoryTradeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameDatabaseInstance() = 0;
    [[nodiscard]] virtual std::uint32_t FindBuyRecord(
        std::uint32_t table, std::uint32_t item_id) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void UpdateCounter(std::uint32_t inventory,
                               std::uint32_t signed_delta_bits) = 0;
    virtual void AddItem(std::uint32_t collection,
                         std::uint32_t item_id) = 0;
    [[nodiscard]] virtual std::uint32_t FindSellRecord(
        std::uint32_t collection, std::uint32_t item_id,
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t QuarterPrice(
        std::uint32_t signed_price_bits) = 0;
    [[nodiscard]] virtual std::int32_t RemoveItem(
        std::uint32_t collection, std::uint32_t record) = 0;
};

class InventoryTrade final {
public:
    static constexpr std::uint32_t kBuyTableOffset = 88u;
    static constexpr std::uint32_t kCollectionOffset = 124u;
    static constexpr std::uint32_t kBalanceOffset = 308u;
    static constexpr std::uint32_t kPriceOffset = 376u;

    // IDA 0x50fed0.
    [[nodiscard]] static std::int32_t Buy(
        std::uint32_t item_id, IInventoryTradeRuntime& runtime);
    // IDA 0x50ff40.
    [[nodiscard]] static std::uint8_t Sell(
        std::uint32_t item_id, IInventoryTradeRuntime& runtime);
};

}  // namespace pal4
