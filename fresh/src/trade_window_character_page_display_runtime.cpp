#include "pal4/trade_window_character_page_display_runtime.h"

namespace pal4 {

std::uint32_t TradeWindowCharacterPageDisplayRuntime::ReadWord(
    const std::uint32_t address) { return pages_.ReadWord(address); }
void TradeWindowCharacterPageDisplayRuntime::WriteWord(
    const std::uint32_t address, const std::uint32_t value) {
    pages_.WriteWord(address, value);
}
void TradeWindowCharacterPageDisplayRuntime::WriteByte(
    const std::uint32_t address, const std::uint8_t value) {
    pages_.WriteByte(address, value);
}
bool TradeWindowCharacterPageDisplayRuntime::IsTradeList(
    const std::uint32_t list) { return pages_.IsTradeList(list); }
void TradeWindowCharacterPageDisplayRuntime::SetImage(
    const std::uint32_t window, const std::uint32_t image) {
    pages_.SetImage(window, image);
}
void TradeWindowCharacterPageDisplayRuntime::SetNamedImage(
    const std::uint32_t window, const std::string_view imageset,
    const std::string_view image) {
    pages_.SetNamedImage(window, imageset, image);
}
void TradeWindowCharacterPageDisplayRuntime::ResetList(const std::uint32_t list) {
    pages_.ResetList(list);
}
std::uint32_t TradeWindowCharacterPageDisplayRuntime::ColumnCount(
    const std::uint32_t list) { return pages_.ColumnCount(list); }
void TradeWindowCharacterPageDisplayRuntime::RemoveColumn(
    const std::uint32_t list, const std::uint32_t column) {
    pages_.RemoveColumn(list, column);
}
void TradeWindowCharacterPageDisplayRuntime::AddColumn(
    const std::uint32_t list, const std::uint32_t column, const float width) {
    pages_.AddColumn(list, column, width);
}
void TradeWindowCharacterPageDisplayRuntime::SetColumnLabelImage(
    const std::uint32_t list, const std::uint32_t column,
    const std::string_view imageset, const std::string_view label) {
    pages_.SetColumnLabelImage(list, column, imageset, label);
}
std::int32_t TradeWindowCharacterPageDisplayRuntime::InventorySize(
    const std::uint32_t page) { return pages_.InventorySize(page); }
std::uint32_t TradeWindowCharacterPageDisplayRuntime::InventoryElement(
    const std::uint32_t page, const std::uint32_t reverse_index) {
    return pages_.InventoryElement(page, reverse_index);
}
std::uint32_t TradeWindowCharacterPageDisplayRuntime::FindInventoryElement(
    const std::uint32_t id, const std::uint32_t page) {
    return pages_.FindInventoryElement(id, page);
}
std::uint32_t TradeWindowCharacterPageDisplayRuntime::InventoryBase() {
    return pages_.InventoryBase();
}
std::string TradeWindowCharacterPageDisplayRuntime::Utf8Name(
    const std::uint32_t name) { return pages_.Utf8Name(name); }
void TradeWindowCharacterPageDisplayRuntime::AddRow(const std::uint32_t list) {
    pages_.AddRow(list);
}
std::uint32_t TradeWindowCharacterPageDisplayRuntime::RowCount(
    const std::uint32_t list) { return pages_.RowCount(list); }
std::uint32_t TradeWindowCharacterPageDisplayRuntime::CreateTextItem(
    const std::uint32_t bytes, const std::uint32_t list,
    const std::string_view text) {
    return pages_.CreateTextItem(bytes, list, text);
}
std::uint32_t TradeWindowCharacterPageDisplayRuntime::CreateSpinnerItem(
    const std::uint32_t bytes) { return pages_.CreateSpinnerItem(bytes); }
void TradeWindowCharacterPageDisplayRuntime::SetItem(
    const std::uint32_t list, const std::uint32_t item,
    const std::uint32_t column, const std::uint32_t row) {
    pages_.SetItem(list, item, column, row);
}
void TradeWindowCharacterPageDisplayRuntime::SetSpinnerRange(
    const std::uint32_t item, const std::int32_t minimum,
    const std::int32_t maximum) {
    pages_.SetSpinnerRange(item, minimum, maximum);
}
void TradeWindowCharacterPageDisplayRuntime::BindSpinnerState(
    const std::uint32_t item) { pages_.BindSpinnerState(item); }
void TradeWindowCharacterPageDisplayRuntime::SetHoverRowIndex(
    const std::uint32_t list, const std::int32_t row) {
    pages_.SetHoverRowIndex(list, row);
}
void TradeWindowCharacterPageDisplayRuntime::ClearItemDisplay(
    const std::uint32_t frame) {
    (void)TradeWindowCharacterItemDisplay::UpdateRecord(frame, 0u, display_);
}
std::uint32_t TradeWindowCharacterPageDisplayRuntime::CleanupPageBuffers() {
    return pages_.CleanupPageBuffers();
}

}  // namespace pal4
