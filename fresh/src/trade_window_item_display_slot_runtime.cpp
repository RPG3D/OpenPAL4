#include "pal4/trade_window_item_display_slot_runtime.h"

namespace pal4 {

std::uint32_t TradeWindowItemDisplaySlotRuntime::ReadWord(
    std::uint32_t address) {
    return display_.ReadWord(address);
}

std::uint8_t TradeWindowItemDisplaySlotRuntime::ReadByte(
    std::uint32_t address) {
    return display_.ReadByte(address);
}

std::uint32_t TradeWindowItemDisplaySlotRuntime::PlayerByIndex(
    std::uint32_t index) {
    return display_.PlayerByIndex(index);
}

void TradeWindowItemDisplaySlotRuntime::UpdatePlayerSlot(
    std::uint32_t slot, std::uint32_t player_index,
    std::uint32_t item) {
    (void)TradeWindowPlayerSlot::Update(slot, player_index, item, slots_);
}

void TradeWindowItemDisplaySlotRuntime::HidePlayerSlot(
    std::uint32_t slot) {
    TradeWindowPlayerSlot::Hide(slot, slots_);
}

std::uint32_t TradeWindowItemDisplaySlotRuntime::InventoryBase() {
    return display_.InventoryBase();
}

std::string TradeWindowItemDisplaySlotRuntime::ReadText(
    std::uint32_t address) {
    return display_.ReadText(address);
}

std::string TradeWindowItemDisplaySlotRuntime::ConvertToUtf8(
    std::string_view text) {
    return display_.ConvertToUtf8(text);
}

std::string TradeWindowItemDisplaySlotRuntime::BuildDescription(
    std::uint32_t item) {
    return display_.BuildDescription(item);
}

void TradeWindowItemDisplaySlotRuntime::SetWindowText(
    std::uint32_t window, std::string_view text) {
    display_.SetWindowText(window, text);
}

void TradeWindowItemDisplaySlotRuntime::SetGameObjectDisplay(
    std::uint32_t frame, std::optional<std::string_view> name,
    std::uint32_t texture, std::optional<std::string_view> path) {
    display_.SetGameObjectDisplay(frame, name, texture, path);
}

std::uint32_t TradeWindowItemDisplaySlotRuntime::CleanupTextBuffers() {
    return display_.CleanupTextBuffers();
}

}  // namespace pal4
