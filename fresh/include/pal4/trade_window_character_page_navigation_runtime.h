#pragma once

#include "pal4/trade_window_character_navigation.h"
#include "pal4/trade_window_character_pages.h"

namespace pal4 {

class TradeWindowCharacterPageNavigationRuntime final
    : public ITradeWindowCharacterNavigationRuntime {
public:
    TradeWindowCharacterPageNavigationRuntime(
        ITradeWindowCharacterNavigationRuntime& navigation,
        ITradeWindowCharacterPageRuntime& pages)
        : navigation_(navigation), pages_(pages) {}

    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void FillActiveCharacter(std::uint32_t frame,
                             std::uint32_t index) override;
    void FillInactiveCharacter(std::uint32_t frame,
                               std::uint32_t index) override;
    [[nodiscard]] std::uint32_t RowCount(std::uint32_t list) override;
    [[nodiscard]] std::uint32_t NextCharacterIndex(
        std::uint32_t index) override;
    [[nodiscard]] std::uint32_t PreviousCharacterIndex(
        std::uint32_t index) override;
    [[nodiscard]] bool HasTradeItems(std::uint32_t frame) override;
    [[nodiscard]] bool CanExecuteTrade(std::uint32_t frame) override;
    void SetVisible(std::uint32_t window, bool visible) override;

private:
    ITradeWindowCharacterNavigationRuntime& navigation_;
    ITradeWindowCharacterPageRuntime& pages_;
};

}  // namespace pal4
