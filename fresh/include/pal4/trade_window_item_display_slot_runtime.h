#pragma once

#include "pal4/trade_window_item_display.h"
#include "pal4/trade_window_player_slot.h"

namespace pal4 {

class TradeWindowItemDisplaySlotRuntime final
    : public ITradeWindowItemDisplayRuntime {
public:
    TradeWindowItemDisplaySlotRuntime(
        ITradeWindowItemDisplayRuntime& display,
        ITradeWindowPlayerSlotRuntime& slots)
        : display_(display), slots_(slots) {}

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t PlayerByIndex(std::uint32_t index) override;
    void UpdatePlayerSlot(std::uint32_t slot, std::uint32_t player_index,
                          std::uint32_t item) override;
    void HidePlayerSlot(std::uint32_t slot) override;
    [[nodiscard]] std::uint32_t InventoryBase() override;
    [[nodiscard]] std::string ReadText(std::uint32_t address) override;
    [[nodiscard]] std::string ConvertToUtf8(std::string_view text) override;
    [[nodiscard]] std::string BuildDescription(std::uint32_t item) override;
    void SetWindowText(std::uint32_t window,
                       std::string_view text) override;
    void SetGameObjectDisplay(std::uint32_t frame,
                              std::optional<std::string_view> name,
                              std::uint32_t texture,
                              std::optional<std::string_view> path) override;
    [[nodiscard]] std::uint32_t CleanupTextBuffers() override;

private:
    ITradeWindowItemDisplayRuntime& display_;
    ITradeWindowPlayerSlotRuntime& slots_;
};

}  // namespace pal4
