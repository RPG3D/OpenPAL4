#pragma once
#include "pal4/native_common_dialog_layout.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeTradeWindowLayout;class NativeInventorySession;class NativeInventoryTrade;
class NativeNumericSpinnerPool;class NativeGameDatabase;class NativePrescriptionRecordCatalog;
struct NativeTradeConfirmationServices {
    NativeCommonDialogLayout::SoundPlayer sound;
    std::function<void(std::uint32_t group,std::uint32_t page,std::uint32_t flag)> refresh_page;
    std::function<void(std::uint32_t definition)> refresh_equipment_display;
    std::function<bool(std::uint32_t instance)> equipped;
};
// Original prompt and transaction algorithms, borrowing the existing list,
// pooled quantities, inventory transactions and shared CommonDialog.
class NativeTradeWindowConfirmation final {
public:
    NativeTradeWindowConfirmation(NativeTradeWindowLayout&,NativeInventorySession&,NativeInventoryTrade&,
        NativeNumericSpinnerPool&,const NativePrescriptionRecordCatalog&,const NativeGameDatabase&,
        NativeCommonDialogLayout&,std::uint32_t manager,NativeTradeConfirmationServices);
    ~NativeTradeWindowConfirmation();
    NativeTradeWindowConfirmation(const NativeTradeWindowConfirmation&)=delete;
    NativeTradeWindowConfirmation& operator=(const NativeTradeWindowConfirmation&)=delete;
    bool Prompt();
    bool ItemClicked(std::uint32_t event);
    void Cancel();
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
