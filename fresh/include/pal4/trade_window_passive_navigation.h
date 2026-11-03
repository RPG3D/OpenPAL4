#pragma once

#include <cstdint>

namespace pal4 {

class ITradeWindowPassiveNavigationRuntime {
public:
    virtual ~ITradeWindowPassiveNavigationRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void ShowActivePage(std::uint32_t frame,
                                std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t NextIndex(
        std::uint32_t frame, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t PreviousIndex(
        std::uint32_t frame, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool HasTradeItems(std::uint32_t frame) = 0;
    [[nodiscard]] virtual bool CanExecuteTrade(std::uint32_t frame) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
};

class TradeWindowPassiveNavigation final {
public:
    // IDA 0x4915a0.
    static void Update(std::uint32_t frame, std::uint32_t first_index,
                       bool forward,
                       ITradeWindowPassiveNavigationRuntime& runtime);
};

}  // namespace pal4
