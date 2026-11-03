#include "pal4/trade_window_character_page_navigation_runtime.h"

namespace pal4 {

std::uint8_t TradeWindowCharacterPageNavigationRuntime::ReadByte(
    const std::uint32_t address) {
    return navigation_.ReadByte(address);
}

std::uint32_t TradeWindowCharacterPageNavigationRuntime::ReadWord(
    const std::uint32_t address) {
    return navigation_.ReadWord(address);
}

void TradeWindowCharacterPageNavigationRuntime::FillActiveCharacter(
    const std::uint32_t frame, const std::uint32_t index) {
    (void)TradeWindowCharacterPages::UpdateActive(frame, index, pages_);
}

void TradeWindowCharacterPageNavigationRuntime::FillInactiveCharacter(
    const std::uint32_t frame, const std::uint32_t index) {
    (void)TradeWindowCharacterPages::Update(frame, index, pages_);
}

std::uint32_t TradeWindowCharacterPageNavigationRuntime::RowCount(
    const std::uint32_t list) {
    return navigation_.RowCount(list);
}

std::uint32_t TradeWindowCharacterPageNavigationRuntime::NextCharacterIndex(
    const std::uint32_t index) {
    return navigation_.NextCharacterIndex(index);
}

std::uint32_t TradeWindowCharacterPageNavigationRuntime::PreviousCharacterIndex(
    const std::uint32_t index) {
    return navigation_.PreviousCharacterIndex(index);
}

bool TradeWindowCharacterPageNavigationRuntime::HasTradeItems(
    const std::uint32_t frame) {
    return navigation_.HasTradeItems(frame);
}

bool TradeWindowCharacterPageNavigationRuntime::CanExecuteTrade(
    const std::uint32_t frame) {
    return navigation_.CanExecuteTrade(frame);
}

void TradeWindowCharacterPageNavigationRuntime::SetVisible(
    const std::uint32_t window, const bool visible) {
    navigation_.SetVisible(window, visible);
}

}  // namespace pal4
