#pragma once

#include "pal4/trade_window_passive_navigation.h"
#include "pal4/trade_window_passive_pages.h"

namespace pal4 {

class TradeWindowPassivePageNavigationRuntime final
    : public ITradeWindowPassiveNavigationRuntime {
public:
    TradeWindowPassivePageNavigationRuntime(
        ITradeWindowPassiveNavigationRuntime& navigation,
        ITradeWindowPassivePageRuntime& pages)
        : navigation_(navigation), pages_(pages) {}

    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void ShowActivePage(std::uint32_t frame, std::uint32_t index) override;
    [[nodiscard]] std::uint32_t RowCount(std::uint32_t list) override;
    [[nodiscard]] std::uint32_t NextIndex(
        std::uint32_t frame, std::uint32_t index) override;
    [[nodiscard]] std::uint32_t PreviousIndex(
        std::uint32_t frame, std::uint32_t index) override;
    [[nodiscard]] bool HasTradeItems(std::uint32_t frame) override;
    [[nodiscard]] bool CanExecuteTrade(std::uint32_t frame) override;
    void SetVisible(std::uint32_t window, bool visible) override;

private:
    ITradeWindowPassiveNavigationRuntime& navigation_;
    ITradeWindowPassivePageRuntime& pages_;
};

}  // namespace pal4
