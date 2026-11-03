#pragma once
#include "pal4/smith_window.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeSmithWindowLayout;class NativeSmithWindowLists;class NativeInventorySession;
class NativePrescriptionRecordCatalog;class NativeGameDatabase;class NativeCommonDialogLayout;class NativeEventHandlerArray;
struct NativeSmithConfirmationServices {
    std::function<bool()> smith_active;
    std::function<void(std::string_view,std::uint32_t)> sound;
    // Required gameplay callback dispatcher. CommonDialog copies the handler;
    // choices retain the original 1=yes contract and include cancellation.
    std::function<void(SmithWindowTradeCallback,std::uint32_t)> selection;
};
// IDA49D5E0 gate and49B2E0 selection panel, borrowing persistent game/UI owners.
// Close before lists/layout; releases only this owner's CommonDialog request.
class NativeSmithWindowConfirmation final {
public:
    NativeSmithWindowConfirmation(NativeSmithWindowLayout&,NativeSmithWindowLists&,NativeInventorySession&,
        const NativePrescriptionRecordCatalog&,const NativeGameDatabase&,NativeCommonDialogLayout&,
        NativeEventHandlerArray&,std::uint32_t manager,NativeSmithConfirmationServices);
    ~NativeSmithWindowConfirmation();
    NativeSmithWindowConfirmation(const NativeSmithWindowConfirmation&)=delete;
    NativeSmithWindowConfirmation& operator=(const NativeSmithWindowConfirmation&)=delete;
    bool Confirm();
    std::uint64_t ShowMessage(std::string_view,std::uint32_t style,std::function<void(std::uint32_t)>);
    void CancelMessage(std::uint64_t generation);
    void ResumeAfterDialog(std::uint32_t choice);
    std::uint32_t UiManagerAddress()const noexcept;
    std::string RequirementsText(std::uint32_t recipe);
    void HideSelection();
    void CancelDialog();
    void SetCategoryHighlighted(bool);
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t activity,std::uint32_t slot);
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
