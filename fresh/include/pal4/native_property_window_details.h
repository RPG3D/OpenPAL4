#pragma once
#include "pal4/property_window_lifecycle.h"
#include <windows.h>
#include <functional>
#include <memory>

namespace pal4 {
class NativeInventorySession;
// Borrows the layout/inventory and required model-preview endpoint. Binds the
// real DLL HoverRowChanged event; retire before the layout or borrowed services.
class NativePropertyWindowDetails final {
public:
    using UpdateModel=std::function<std::uint32_t(PropertyWindowState&,std::uint32_t record)>;
    NativePropertyWindowDetails(HMODULE,PropertyWindowState&,NativeInventorySession&,UpdateModel);
    ~NativePropertyWindowDetails();
    NativePropertyWindowDetails(const NativePropertyWindowDetails&)=delete;
    NativePropertyWindowDetails& operator=(const NativePropertyWindowDetails&)=delete;
    void UpdateHovered();
    void Clear();
    void RethrowEventError();
    // Disables the DLL subscriber. Preview ownership belongs to the page owner.
    void Close();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
