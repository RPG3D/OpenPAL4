#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ITradeWindowPassivePageRuntime {
public:
    virtual ~ITradeWindowPassivePageRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
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
    [[nodiscard]] virtual std::uint32_t FindGameDbItem(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindEffect(
        std::uint32_t game_db_item) = 0;
    [[nodiscard]] virtual std::uint32_t InventorySearch(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t InventoryBase() = 0;
    [[nodiscard]] virtual std::string Utf8Name(std::uint32_t name) = 0;
    virtual void AddRow(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t CreateItem(
        std::uint32_t bytes, std::uint32_t list,
        std::string_view text) = 0;
    virtual void SetItem(std::uint32_t list, std::uint32_t item,
                         std::uint32_t column, std::uint32_t row) = 0;
    virtual void SetHoverRowIndex(std::uint32_t list,
                                  std::int32_t row) = 0;
    virtual void ClearItemDisplay(std::uint32_t frame) = 0;
};

class TradeWindowPassivePages final {
public:
    // IDA 0x491640.
    static void Update(std::uint32_t frame, std::uint32_t page,
                       ITradeWindowPassivePageRuntime& runtime);
};

}  // namespace pal4
