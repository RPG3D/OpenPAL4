#include "pal4/trade_window_passive_page_navigation_runtime.h"
#include "pal4/trade_window_passive_page_index.h"

#include <bit>

namespace pal4 {

std::uint8_t TradeWindowPassivePageNavigationRuntime::ReadByte(
    const std::uint32_t address) {
    return navigation_.ReadByte(address);
}

std::uint32_t TradeWindowPassivePageNavigationRuntime::ReadWord(
    const std::uint32_t address) {
    return navigation_.ReadWord(address);
}

void TradeWindowPassivePageNavigationRuntime::ShowActivePage(
    const std::uint32_t frame, const std::uint32_t index) {
    TradeWindowPassivePages::Update(frame, index, pages_);
}

std::uint32_t TradeWindowPassivePageNavigationRuntime::RowCount(
    const std::uint32_t list) {
    return navigation_.RowCount(list);
}

std::uint32_t TradeWindowPassivePageNavigationRuntime::NextIndex(
    const std::uint32_t frame, const std::uint32_t index) {
    (void)frame;
    return std::bit_cast<std::uint32_t>(
        TradeWindowPassivePageIndex::Next(
            std::bit_cast<std::int32_t>(index)));
}

std::uint32_t TradeWindowPassivePageNavigationRuntime::PreviousIndex(
    const std::uint32_t frame, const std::uint32_t index) {
    (void)frame;
    return std::bit_cast<std::uint32_t>(
        TradeWindowPassivePageIndex::Previous(
            std::bit_cast<std::int32_t>(index)));
}

bool TradeWindowPassivePageNavigationRuntime::HasTradeItems(
    const std::uint32_t frame) {
    return navigation_.HasTradeItems(frame);
}

bool TradeWindowPassivePageNavigationRuntime::CanExecuteTrade(
    const std::uint32_t frame) {
    return navigation_.CanExecuteTrade(frame);
}

void TradeWindowPassivePageNavigationRuntime::SetVisible(
    const std::uint32_t window, const bool visible) {
    navigation_.SetVisible(window, visible);
}

}  // namespace pal4
