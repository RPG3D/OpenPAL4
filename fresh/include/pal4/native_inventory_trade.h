#pragma once
#include "pal4/inventory_trade.h"
#include "pal4/inventory_transactions.h"
#include <span>
namespace pal4 {
class NativeInventorySession;
class NativePrescriptionRecordCatalog;
// All operations mutate the existing inventory. Equipment sale keys identify
// instances, property sale keys identify inventory sequences, recipe keys identify DB rows.
class NativeInventoryTrade final : private IInventoryTradeRuntime,
                                   private IInventoryTransactionsRuntime {
public:
    NativeInventoryTrade(NativeInventorySession&,const NativePrescriptionRecordCatalog&);
    std::int32_t BuyEquipment(std::uint32_t id);
    std::uint8_t SellEquipment(std::uint32_t sequence);
    std::int32_t BuyProperty(std::uint32_t id,std::uint32_t quantity);
    std::int32_t SellProperty(std::uint32_t sequence,std::uint32_t quantity);
    std::int32_t BuyProperties(std::span<const ItemQuantity>);
    std::int32_t SellProperties(std::span<const ItemQuantity>);
    std::int32_t BuyRecipe(std::uint32_t id);
private:
    std::uint32_t GameDatabaseInstance() override;
    std::uint32_t FindBuyRecord(std::uint32_t,std::uint32_t) override;
    std::uint32_t Inventory() override;
    std::uint32_t ReadWord(std::uint32_t) override;
    void UpdateCounter(std::uint32_t,std::uint32_t) override;
    void AddItem(std::uint32_t,std::uint32_t) override;
    std::uint32_t FindSellRecord(std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t QuarterPrice(std::uint32_t) override;
    std::int32_t RemoveItem(std::uint32_t,std::uint32_t) override;
    std::uint32_t FindItemRecord(std::uint32_t,std::uint32_t) override;
    std::uint32_t FindRecipeRecord(std::uint32_t,std::uint32_t) override;
    void ProcessItem(std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t FindInventoryEntry(std::uint32_t,std::uint32_t,std::uint32_t) override;
    void RemoveInventoryItem(std::uint32_t,std::uint32_t,std::uint32_t) override;
    bool HasRecipe(std::uint32_t,std::uint32_t) override;
    void LearnRecipe(std::uint32_t,std::uint32_t,std::uint32_t) override;
    NativeInventorySession& inventory_;
    const NativePrescriptionRecordCatalog& recipes_;
};
}
