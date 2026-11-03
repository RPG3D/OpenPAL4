#pragma once

#include "pal4/trade_window_repeat.h"
#include "pal4/trade_window_confirmation.h"
#include "pal4/trade_window_total_price.h"
#include "pal4/trade_window_category.h"
#include "pal4/trade_window_hover.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace pal4 {

class ITradeWindowTradeRuntime : public ITradeWindowQuantityRuntime,public ITradeWindowConfirmationRuntime,public ITradeWindowTotalPriceRuntime,public ITradeWindowCategoryRuntime,public ITradeWindowHoverRuntime {
public:
    virtual ~ITradeWindowTradeRuntime() = default;
    void RefreshTotalPrice(std::uint32_t frame) override;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetPassiveFromHover(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t GetItemFromHover(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t GetInventoryFromHover(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t GetBuyItemFromHover(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t GetBuyInventoryFromHover(
        std::uint32_t frame) = 0;
    virtual void InvokeHoverHandler(std::uint32_t address,
                                    std::uint32_t frame,
                                    std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    [[nodiscard]] virtual std::uint32_t LearnRecipe(
        std::uint32_t trade, std::uint32_t recipe) = 0;
    [[nodiscard]] virtual std::uint32_t BuyItem(
        std::uint32_t trade, std::uint32_t item) = 0;
    virtual void SellItem(std::uint32_t trade, std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t AddMultipleItems(
        std::uint32_t trade, const std::vector<TradeBatchEntry>& items) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveMultipleItems(
        std::uint32_t trade, const std::vector<TradeBatchEntry>& items) = 0;
    [[nodiscard]] virtual std::int32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t GridItem(
        std::uint32_t list, std::int32_t row, std::int32_t column) = 0;
    [[nodiscard]] virtual std::int32_t SpinnerValue(std::uint32_t item) = 0;
    virtual void SetSpinnerValue(std::uint32_t item,
                                 std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t ResolveCharacterItem(
        std::uint32_t database, std::uint32_t id) = 0;
    [[nodiscard]] virtual bool ContainsCharacterItem(
        std::uint32_t database, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t InventorySubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryEntry(
        std::uint32_t inventory, std::uint32_t id,
        std::uint32_t selector) = 0;
    virtual void InsertBatchRecord(std::vector<TradeBatchEntry>& items,
                                   TradeBatchEntry entry) = 0;
    virtual void UpdatePage(std::uint32_t frame, std::uint32_t mode,
                            std::uint32_t selected, std::uint32_t flag) = 0;
    virtual void UpdateItemDisplay(std::uint32_t frame,
                                   std::uint32_t item) = 0;
    virtual void SetHoverRowIndex(std::uint32_t list,
                                  std::int32_t row) = 0;
    virtual void SetScrollPosition(std::uint32_t scrollbar,
                                   float position) = 0;
    virtual void CreateCommonDialog(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::string_view TriggerText(
        std::uint32_t trigger) = 0;
    virtual void ShowDialog(std::string_view text, float x, float y,
                            std::uint32_t type, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void RemoveObjectByIdentifier(std::uint32_t collection,
                                          std::uint32_t identifier) = 0;
    virtual void AddTimerHandler(std::uint32_t collection,
                                 std::uint32_t identifier,
                                 std::uint32_t delay_bits,
                                 std::uint32_t frame,
                                 std::uint32_t handler,
                                 std::int32_t row,
                                 std::uint8_t decrement) = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t AdjacentIndex(
        std::uint32_t mode, std::uint32_t current,
        bool next) = 0;
    [[nodiscard]] virtual bool HasInventoryRecipe(
        std::uint32_t collection, std::uint32_t recipe) = 0;
    [[nodiscard]] virtual bool CheckPlayersCondition(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void ReleaseCallback(std::uint32_t callback) = 0;
};

class TradeWindowTradeActions final {
public:
    // IDA 0x48c600.
    [[nodiscard]] static bool OnTradeClick(
        std::uint32_t frame, std::uint32_t event,
        ITradeWindowConfirmationRuntime& runtime);
    // IDA 0x48cd20.
    static void ModifyItemCount(std::uint32_t frame, std::int32_t row,
                                std::uint8_t decrement,
                                ITradeWindowQuantityRuntime& runtime);
    // IDA 0x48cd80.
    static void OnListClick(std::uint32_t frame, std::int32_t row,
                            std::uint8_t decrement,
                            ITradeWindowQuantityRuntime& runtime);
    // IDA 0x48cf80.
    static void UpdateTotalPrice(std::uint32_t frame,
                                 ITradeWindowTotalPriceRuntime& runtime);
    // IDA 0x48d1d0/0x48d290/0x48d310.
    [[nodiscard]] static bool OnHoverChanged(
        std::uint32_t frame, ITradeWindowHoverRuntime& runtime);
    [[nodiscard]] static bool OnPrevButton(
        std::uint32_t frame, ITradeWindowCategoryRuntime& runtime);
    [[nodiscard]] static bool OnNextButton(
        std::uint32_t frame, ITradeWindowCategoryRuntime& runtime);
    // IDA 0x48d1a0/0x48d390/0x48d3a0.
    [[nodiscard]] static bool OnItemHover(
        std::uint32_t frame, std::uint32_t event,
        ITradeWindowConfirmationRuntime& runtime);
    [[nodiscard]] static bool OnItemTooltip(
        std::uint32_t frame, ITradeWindowConfirmationRuntime& runtime);
    [[nodiscard]] static bool ShowTradePrompt(
        std::uint32_t frame, ITradeWindowConfirmationRuntime& runtime);
};

inline void ITradeWindowTradeRuntime::RefreshTotalPrice(std::uint32_t frame){TradeWindowTradeActions::UpdateTotalPrice(frame,*this);}

}  // namespace pal4
