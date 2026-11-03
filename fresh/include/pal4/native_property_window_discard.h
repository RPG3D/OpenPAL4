#pragma once
#include "pal4/native_common_dialog_layout.h"
#include "pal4/property_window_lifecycle.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeGameDatabase;
class NativeInventorySession;
// Owns the pending discard operation, borrowing the shared confirmation dialog.
// The page, inventory, database and dialog must outlive Close. Sound policy is
// required; bounded tests supply a recorder. Refresh uses the page's own list.
class NativePropertyWindowDiscard final {
public:
    using Refresh=std::function<void(PropertyWindowState&,std::uint32_t,bool)>;
    NativePropertyWindowDiscard(NativeInventorySession&,const NativeGameDatabase&,
        NativeCommonDialogLayout&,NativeCommonDialogLayout::SoundPlayer,Refresh);
    ~NativePropertyWindowDiscard();
    NativePropertyWindowDiscard(const NativePropertyWindowDiscard&)=delete;
    NativePropertyWindowDiscard& operator=(const NativePropertyWindowDiscard&)=delete;
    bool Show(PropertyWindowState&,std::uint32_t item_address);
    void Close();
private:
    class Runtime;
    std::shared_ptr<Runtime> runtime_;
};
}
