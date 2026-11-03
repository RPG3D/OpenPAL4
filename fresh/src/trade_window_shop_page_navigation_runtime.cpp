#include "pal4/trade_window_shop_page_navigation_runtime.h"

namespace pal4 {

std::uint32_t TradeWindowShopPageNavigationRuntime::ReadWord(
    const std::uint32_t address) {
    return navigation_.ReadWord(address);
}

std::uint8_t TradeWindowShopPageNavigationRuntime::ReadByte(
    const std::uint32_t address) {
    return navigation_.ReadByte(address);
}

void TradeWindowShopPageNavigationRuntime::ShowActivePage(
    const std::uint32_t frame, const std::uint32_t index) {
    (void)TradeWindowShopPages::UpdateActive(frame, index, pages_);
}

void TradeWindowShopPageNavigationRuntime::ShowInactivePage(
    const std::uint32_t frame, const std::uint32_t index) {
    (void)TradeWindowShopPages::UpdateInactive(frame, index, pages_);
}

std::uint32_t TradeWindowShopPageNavigationRuntime::RowCount(
    const std::uint32_t list) {
    return navigation_.RowCount(list);
}

std::uint32_t TradeWindowShopPageNavigationRuntime::NextIndex(
    const std::uint32_t frame, const std::uint32_t index) {
    return navigation_.NextIndex(frame, index);
}

std::uint32_t TradeWindowShopPageNavigationRuntime::PrevIndex(
    const std::uint32_t frame, const std::uint32_t index) {
    return navigation_.PrevIndex(frame, index);
}

bool TradeWindowShopPageNavigationRuntime::HasTradeItems(
    const std::uint32_t frame) {
    return navigation_.HasTradeItems(frame);
}

bool TradeWindowShopPageNavigationRuntime::CanExecuteTrade(
    const std::uint32_t frame) {
    return navigation_.CanExecuteTrade(frame);
}

void TradeWindowShopPageNavigationRuntime::SetVisible(
    const std::uint32_t window, const bool visible) {
    navigation_.SetVisible(window, visible);
}

}  // namespace pal4
