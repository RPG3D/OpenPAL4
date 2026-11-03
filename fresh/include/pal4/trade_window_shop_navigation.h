#pragma once

#include <cstdint>

namespace pal4 {

class ITradeWindowShopNavigationRuntime {
public:
    virtual ~ITradeWindowShopNavigationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void ShowActivePage(std::uint32_t frame,
                                std::uint32_t index) = 0;
    virtual void ShowInactivePage(std::uint32_t frame,
                                  std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t NextIndex(
        std::uint32_t frame, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t PrevIndex(
        std::uint32_t frame, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool HasTradeItems(std::uint32_t frame) = 0;
    [[nodiscard]] virtual bool CanExecuteTrade(std::uint32_t frame) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
};

class TradeWindowShopNavigation final {
public:
    // IDA 0x48ded0.
    static void UpdateShopData(std::uint32_t frame,
                               std::uint32_t first_index,
                               bool forward,
                               ITradeWindowShopNavigationRuntime& runtime);
};

}  // namespace pal4
