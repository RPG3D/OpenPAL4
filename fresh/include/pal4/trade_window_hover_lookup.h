#pragma once

#include <cstdint>

namespace pal4 {

class ITradeWindowHoverLookupRuntime {
public:
    virtual ~ITradeWindowHoverLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetHoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t GameDbBase() = 0;
    [[nodiscard]] virtual std::uint32_t InventoryBase() = 0;
    [[nodiscard]] virtual std::uint32_t FindGameDbItem(
        std::uint32_t collection, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindGameDbPassiveItem(
        std::uint32_t collection, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindGameDbCharacterItem(
        std::uint32_t database, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryItem(
        std::uint32_t collection, std::uint32_t id,
        std::uint32_t category) = 0;
};

class TradeWindowHoverLookup final {
public:
    // IDA 0x4906f0/0x490730/0x493e90/0x496910/0x496940.
    [[nodiscard]] static std::uint32_t GetGameDbItem(
        std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetInventoryItem(
        std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetPassiveItem(
        std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetBuyItem(
        std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetBuyInventoryItem(
        std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime);
};

class TradeWindowPageIndex final {
public:
    // IDA 0x490770/0x490780.
    [[nodiscard]] static std::int32_t Next(std::int32_t index);
    [[nodiscard]] static std::int32_t Previous(std::int32_t index);
};

}  // namespace pal4
