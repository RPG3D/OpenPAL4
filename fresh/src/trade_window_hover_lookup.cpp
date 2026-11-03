#include "pal4/trade_window_hover_lookup.h"

#include <bit>
#include <optional>

namespace pal4 {
namespace {

std::optional<std::uint32_t> HoverRecordId(
    std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime) {
    const auto list = runtime.ReadWord(frame + 0x28u);
    const auto hovered = runtime.GetHoverItem(list);
    if (hovered == 0u) return std::nullopt;
    return runtime.ReadWord(hovered + 0x138u);
}

}  // namespace

std::uint32_t TradeWindowHoverLookup::GetGameDbItem(
    std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime) {
    if (runtime.ReadWord(frame + 0xe8u) != 0u) return 0u;
    const auto id = HoverRecordId(frame, runtime);
    if (!id) return 0u;
    return runtime.FindGameDbItem(runtime.GameDbBase() + 0x58u, *id);
}

std::uint32_t TradeWindowHoverLookup::GetInventoryItem(
    std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime) {
    if (runtime.ReadWord(frame + 0xe8u) != 0u) return 0u;
    const auto id = HoverRecordId(frame, runtime);
    if (!id) return 0u;
    return runtime.FindInventoryItem(runtime.InventoryBase() + 0x7cu,
                                     *id, 7u);
}

std::uint32_t TradeWindowHoverLookup::GetPassiveItem(
    std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime) {
    if (runtime.ReadWord(frame + 0xe8u) != 2u) return 0u;
    const auto id = HoverRecordId(frame, runtime);
    if (!id) return 0u;
    return runtime.FindGameDbPassiveItem(
        runtime.GameDbBase() + 0x80u, *id);
}

std::uint32_t TradeWindowHoverLookup::GetBuyItem(
    std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime) {
    if (runtime.ReadWord(frame + 0xe8u) != 1u) return 0u;
    const auto id = HoverRecordId(frame, runtime);
    if (!id) return 0u;
    return runtime.FindGameDbCharacterItem(runtime.GameDbBase(), *id);
}

std::uint32_t TradeWindowHoverLookup::GetBuyInventoryItem(
    std::uint32_t frame, ITradeWindowHoverLookupRuntime& runtime) {
    if (runtime.ReadWord(frame + 0xe8u) != 1u) return 0u;
    const auto id = HoverRecordId(frame, runtime);
    if (!id) return 0u;
    return runtime.FindInventoryItem(runtime.InventoryBase(), *id, 5u);
}

std::int32_t TradeWindowPageIndex::Next(std::int32_t index) {
    const auto value = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(index) + 1u);
    return value >= 7 ? 0 : value;
}

std::int32_t TradeWindowPageIndex::Previous(std::int32_t index) {
    const auto value = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(index) - 1u);
    return value < 0 ? 6 : value;
}

}  // namespace pal4
