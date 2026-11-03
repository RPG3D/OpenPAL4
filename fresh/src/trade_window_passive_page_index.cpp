#include "pal4/trade_window_passive_page_index.h"

#include <bit>

namespace pal4 {

std::int32_t TradeWindowPassivePageIndex::Next(
    const std::int32_t index) {
    const auto next = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(index) + 1u);
    return next >= 3 ? 0 : next;
}

std::int32_t TradeWindowPassivePageIndex::Previous(
    const std::int32_t index) {
    const auto previous = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(index) - 1u);
    return previous < 0 ? 2 : previous;
}

}  // namespace pal4
