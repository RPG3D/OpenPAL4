#pragma once
#include "pal4/smith_window.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeSmithWindowLayout;class NativeSmithWindowLists;class NativeSmithWindowConfirmation;
class NativeInventorySession;class NativePrescriptionRecordCatalog;class NativeGameDatabase;
class NativeCraftingApplication;class NativeCraftingStrengthening;class NativeCraftingEnchantment;
// Original confirmed crafting callbacks; borrows the existing mutation owners.
// Equipment trade/prompt entry uses the original money/potential/Skill gates.
class NativeSmithWindowCraftActions final {
public:
    NativeSmithWindowCraftActions(NativeSmithWindowLayout&,NativeSmithWindowLists&,NativeSmithWindowConfirmation&,
        NativeInventorySession&,const NativePrescriptionRecordCatalog&,const NativeGameDatabase&,
        NativeCraftingApplication&,NativeCraftingStrengthening&,NativeCraftingEnchantment&,std::function<bool()> smith_active);
    ~NativeSmithWindowCraftActions();
    NativeSmithWindowCraftActions(const NativeSmithWindowCraftActions&)=delete;
    NativeSmithWindowCraftActions& operator=(const NativeSmithWindowCraftActions&)=delete;
    bool Dispatch(SmithWindowTradeCallback,std::uint32_t choice);
    bool TradeSelected();
    bool FromDll(std::uint32_t handler);
    void CancelPending();
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
