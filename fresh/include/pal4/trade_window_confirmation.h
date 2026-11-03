#pragma once
#include <cstdint>
#include <string_view>
#include <vector>
namespace pal4 {
struct TradeBatchEntry {
    std::uint32_t item;
    std::int32_t quantity;
};

// Confirmation and resulting transaction boundary, without navigation/effects.
class ITradeWindowConfirmationRuntime {
public:
    virtual ~ITradeWindowConfirmationRuntime()=default;
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
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t ResolveCharacterItem(
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
    [[nodiscard]] virtual bool HasInventoryRecipe(
        std::uint32_t collection, std::uint32_t recipe) = 0;
    [[nodiscard]] virtual bool CheckPlayersCondition(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void ReleaseCallback(std::uint32_t callback) = 0;
};
}
