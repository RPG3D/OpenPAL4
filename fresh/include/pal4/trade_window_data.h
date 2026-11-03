#pragma once
#include <cstdint>
namespace pal4 {
class ITradeWindowDataRuntime {
public:
    virtual ~ITradeWindowDataRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual std::uint8_t ReadByte(std::uint32_t)=0;
    virtual void WriteByte(std::uint32_t,std::uint8_t)=0;
    virtual std::uint32_t FrameManager()=0;
    [[nodiscard]] virtual std::uint32_t CopyShopId(
        std::uint32_t source) = 0;
    virtual void ReleaseShopId(std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t TradeItemCount(std::uint32_t data) = 0;
    [[nodiscard]] virtual std::uint32_t TradeItemAt(
        std::uint32_t data, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool ShopIdDiffers(std::uint32_t item_text,
                                         std::uint32_t attribute) = 0;
    virtual void ClearTradeItems(std::uint32_t frame,
                                 std::uint32_t group) = 0;
    [[nodiscard]] virtual std::uint32_t GameDataOwner() = 0;
    [[nodiscard]] virtual std::int32_t GameDataValue(std::uint32_t game,
                                                   std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveTradeItem(
        std::uint32_t group, std::uint32_t id) = 0;
    virtual void AppendTradeItem(std::uint32_t frame,
                                 std::uint32_t group,
                                 std::uint32_t item) = 0;
    virtual void RemoveActiveFrame(std::uint32_t array,
                                   std::uint32_t callback) = 0;
    virtual void InsertActiveFrame(std::uint32_t array,
                                   std::uint32_t callback) = 0;
    virtual void SetWindowVisible(std::uint32_t window, bool visible) = 0;
    virtual void UpdateTradePage(std::uint32_t frame,
                                 std::uint32_t group,
                                 std::uint32_t a, std::uint32_t b) = 0;
    [[nodiscard]] virtual std::uint32_t InventorySubsystem() = 0;
    [[nodiscard]] virtual std::int32_t InventoryElementSize(
        std::uint32_t collection, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t InventoryCalculatedValue(
        std::uint32_t inventory, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t InventorySubSize(
        std::uint32_t collection, std::uint32_t index) = 0;
};
}
