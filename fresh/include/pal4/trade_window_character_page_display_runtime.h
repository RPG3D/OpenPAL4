#pragma once

#include "pal4/trade_window_character_item_display.h"
#include "pal4/trade_window_character_pages.h"

namespace pal4 {

class TradeWindowCharacterPageDisplayRuntime final
    : public ITradeWindowCharacterPageRuntime {
public:
    TradeWindowCharacterPageDisplayRuntime(
        ITradeWindowCharacterPageRuntime& pages,
        ITradeWindowCharacterItemDisplayRuntime& display)
        : pages_(pages), display_(display) {}

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    [[nodiscard]] bool IsTradeList(std::uint32_t list) override;
    void SetImage(std::uint32_t window, std::uint32_t image) override;
    void SetNamedImage(std::uint32_t window, std::string_view imageset,
                       std::string_view image) override;
    void ResetList(std::uint32_t list) override;
    [[nodiscard]] std::uint32_t ColumnCount(std::uint32_t list) override;
    void RemoveColumn(std::uint32_t list, std::uint32_t column) override;
    void AddColumn(std::uint32_t list, std::uint32_t column,
                   float width) override;
    void SetColumnLabelImage(std::uint32_t list, std::uint32_t column,
                             std::string_view imageset,
                             std::string_view label) override;
    [[nodiscard]] std::int32_t InventorySize(std::uint32_t page) override;
    [[nodiscard]] std::uint32_t InventoryElement(
        std::uint32_t page, std::uint32_t reverse_index) override;
    [[nodiscard]] std::uint32_t FindInventoryElement(
        std::uint32_t id, std::uint32_t page) override;
    [[nodiscard]] std::uint32_t InventoryBase() override;
    [[nodiscard]] std::string Utf8Name(std::uint32_t name) override;
    void AddRow(std::uint32_t list) override;
    [[nodiscard]] std::uint32_t RowCount(std::uint32_t list) override;
    [[nodiscard]] std::uint32_t CreateTextItem(
        std::uint32_t bytes, std::uint32_t list,
        std::string_view text) override;
    [[nodiscard]] std::uint32_t CreateSpinnerItem(
        std::uint32_t bytes) override;
    void SetItem(std::uint32_t list, std::uint32_t item,
                 std::uint32_t column, std::uint32_t row) override;
    void SetSpinnerRange(std::uint32_t item, std::int32_t minimum,
                         std::int32_t maximum) override;
    void BindSpinnerState(std::uint32_t item) override;
    void SetHoverRowIndex(std::uint32_t list, std::int32_t row) override;
    void ClearItemDisplay(std::uint32_t frame) override;
    [[nodiscard]] std::uint32_t CleanupPageBuffers() override;

private:
    ITradeWindowCharacterPageRuntime& pages_;
    ITradeWindowCharacterItemDisplayRuntime& display_;
};

}  // namespace pal4
