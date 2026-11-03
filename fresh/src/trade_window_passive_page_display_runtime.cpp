#include "pal4/trade_window_passive_page_display_runtime.h"

namespace pal4 {

std::uint8_t TradeWindowPassivePageDisplayRuntime::ReadByte(
    const std::uint32_t address) {
    return pages_.ReadByte(address);
}

std::uint32_t TradeWindowPassivePageDisplayRuntime::ReadWord(
    const std::uint32_t address) {
    return pages_.ReadWord(address);
}

void TradeWindowPassivePageDisplayRuntime::WriteWord(
    const std::uint32_t address, const std::uint32_t value) {
    pages_.WriteWord(address, value);
}

void TradeWindowPassivePageDisplayRuntime::WriteByte(
    const std::uint32_t address, const std::uint8_t value) {
    pages_.WriteByte(address, value);
}

bool TradeWindowPassivePageDisplayRuntime::IsTradeList(
    const std::uint32_t list) {
    return pages_.IsTradeList(list);
}

void TradeWindowPassivePageDisplayRuntime::SetImage(
    const std::uint32_t window, const std::uint32_t image) {
    pages_.SetImage(window, image);
}

void TradeWindowPassivePageDisplayRuntime::SetNamedImage(
    const std::uint32_t window, const std::string_view imageset,
    const std::string_view image) {
    pages_.SetNamedImage(window, imageset, image);
}

void TradeWindowPassivePageDisplayRuntime::ResetList(
    const std::uint32_t list) {
    pages_.ResetList(list);
}

std::uint32_t TradeWindowPassivePageDisplayRuntime::ColumnCount(
    const std::uint32_t list) {
    return pages_.ColumnCount(list);
}

void TradeWindowPassivePageDisplayRuntime::RemoveColumn(
    const std::uint32_t list, const std::uint32_t column) {
    pages_.RemoveColumn(list, column);
}

void TradeWindowPassivePageDisplayRuntime::AddColumn(
    const std::uint32_t list, const std::uint32_t column,
    const float width) {
    pages_.AddColumn(list, column, width);
}

void TradeWindowPassivePageDisplayRuntime::SetColumnLabelImage(
    const std::uint32_t list, const std::uint32_t column,
    const std::string_view imageset, const std::string_view label) {
    pages_.SetColumnLabelImage(list, column, imageset, label);
}

std::uint32_t TradeWindowPassivePageDisplayRuntime::FindGameDbItem(
    const std::uint32_t id) {
    return pages_.FindGameDbItem(id);
}

std::uint32_t TradeWindowPassivePageDisplayRuntime::FindEffect(
    const std::uint32_t game_db_item) {
    return pages_.FindEffect(game_db_item);
}

std::uint32_t TradeWindowPassivePageDisplayRuntime::InventorySearch(
    const std::uint32_t id) {
    return pages_.InventorySearch(id);
}

std::uint32_t TradeWindowPassivePageDisplayRuntime::InventoryBase() {
    return pages_.InventoryBase();
}

std::string TradeWindowPassivePageDisplayRuntime::Utf8Name(
    const std::uint32_t name) {
    return pages_.Utf8Name(name);
}

void TradeWindowPassivePageDisplayRuntime::AddRow(
    const std::uint32_t list) {
    pages_.AddRow(list);
}

std::uint32_t TradeWindowPassivePageDisplayRuntime::RowCount(
    const std::uint32_t list) {
    return pages_.RowCount(list);
}

std::uint32_t TradeWindowPassivePageDisplayRuntime::CreateItem(
    const std::uint32_t bytes, const std::uint32_t list,
    const std::string_view text) {
    return pages_.CreateItem(bytes, list, text);
}

void TradeWindowPassivePageDisplayRuntime::SetItem(
    const std::uint32_t list, const std::uint32_t item,
    const std::uint32_t column, const std::uint32_t row) {
    pages_.SetItem(list, item, column, row);
}

void TradeWindowPassivePageDisplayRuntime::SetHoverRowIndex(
    const std::uint32_t list, const std::int32_t row) {
    pages_.SetHoverRowIndex(list, row);
}

void TradeWindowPassivePageDisplayRuntime::ClearItemDisplay(
    const std::uint32_t frame) {
    (void)TradeWindowPassiveItemDisplay::Update(frame, 0u, display_);
}

}  // namespace pal4
