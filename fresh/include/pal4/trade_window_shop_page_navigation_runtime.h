#pragma once

#include "pal4/trade_window_shop_navigation.h"
#include "pal4/trade_window_shop_pages.h"

namespace pal4 {

class TradeWindowShopPageNavigationRuntime final
    : public ITradeWindowShopNavigationRuntime {
public:
    TradeWindowShopPageNavigationRuntime(
        ITradeWindowShopNavigationRuntime& navigation,
        ITradeWindowShopPageRuntime& pages)
        : navigation_(navigation), pages_(pages) {}

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void ShowActivePage(std::uint32_t frame,
                        std::uint32_t index) override;
    void ShowInactivePage(std::uint32_t frame,
                          std::uint32_t index) override;
    [[nodiscard]] std::uint32_t RowCount(std::uint32_t list) override;
    [[nodiscard]] std::uint32_t NextIndex(
        std::uint32_t frame, std::uint32_t index) override;
    [[nodiscard]] std::uint32_t PrevIndex(
        std::uint32_t frame, std::uint32_t index) override;
    [[nodiscard]] bool HasTradeItems(std::uint32_t frame) override;
    [[nodiscard]] bool CanExecuteTrade(std::uint32_t frame) override;
    void SetVisible(std::uint32_t window, bool visible) override;

private:
    ITradeWindowShopNavigationRuntime& navigation_;
    ITradeWindowShopPageRuntime& pages_;
};

}  // namespace pal4
