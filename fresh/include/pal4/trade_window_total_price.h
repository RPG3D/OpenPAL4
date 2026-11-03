#pragma once

// Keep the game interface name independent of Win32 ANSI/Unicode aliases.
#ifdef SetWindowText
#undef SetWindowText
#endif
#include <cstdint>
#include <string_view>
namespace pal4 {
class ITradeWindowTotalPriceRuntime {
public:
    virtual ~ITradeWindowTotalPriceRuntime()=default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t GridItem(
        std::uint32_t list, std::int32_t row, std::int32_t column) = 0;
    [[nodiscard]] virtual std::int32_t SpinnerValue(std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t ResolveCharacterItem(
        std::uint32_t database, std::uint32_t id) = 0;
    [[nodiscard]] virtual bool ContainsCharacterItem(
        std::uint32_t database, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t InventorySubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryEntry(
        std::uint32_t inventory, std::uint32_t id,
        std::uint32_t selector) = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::string_view text) = 0;
};
}
