#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ITradeWindowCharacterPageRuntime {
public:
    virtual ~ITradeWindowCharacterPageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual bool IsTradeList(std::uint32_t list) = 0;
    virtual void SetImage(std::uint32_t window, std::uint32_t image) = 0;
    virtual void SetNamedImage(std::uint32_t window,
                               std::string_view imageset,
                               std::string_view image) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list, std::uint32_t column) = 0;
    virtual void AddColumn(std::uint32_t list, std::uint32_t column,
                           float width) = 0;
    virtual void SetColumnLabelImage(std::uint32_t list,
                                     std::uint32_t column,
                                     std::string_view imageset,
                                     std::string_view label) = 0;
    [[nodiscard]] virtual std::int32_t InventorySize(std::uint32_t page) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryElement(
        std::uint32_t page, std::uint32_t reverse_index) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryElement(
        std::uint32_t id, std::uint32_t page) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryBase() = 0;
    [[nodiscard]] virtual std::string Utf8Name(std::uint32_t name) = 0;
    virtual void AddRow(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTextItem(
        std::uint32_t bytes, std::uint32_t list,
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CreateSpinnerItem(
        std::uint32_t bytes) = 0;
    virtual void SetItem(std::uint32_t list, std::uint32_t item,
                         std::uint32_t column, std::uint32_t row) = 0;
    virtual void SetSpinnerRange(std::uint32_t item,
                                 std::int32_t minimum,
                                 std::int32_t maximum) = 0;
    virtual void BindSpinnerState(std::uint32_t item) = 0;
    virtual void SetHoverRowIndex(std::uint32_t list,
                                  std::int32_t row) = 0;
    virtual void ClearItemDisplay(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPageBuffers() = 0;
};

class TradeWindowCharacterPages final {
public:
    // IDA 0x493fe0.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t frame, std::uint32_t page,
        ITradeWindowCharacterPageRuntime& runtime);
    // IDA 0x495080.
    [[nodiscard]] static std::uint32_t UpdateActive(
        std::uint32_t frame, std::uint32_t page,
        ITradeWindowCharacterPageRuntime& runtime);
};

}  // namespace pal4
