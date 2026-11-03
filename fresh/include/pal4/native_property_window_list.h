#pragma once
#include "pal4/property_window_lifecycle.h"
#include <windows.h>
#include <functional>
#include <memory>

namespace pal4 {
class NativeInventorySession;
// Model/panel and detail endpoints belong to the page owner. Required endpoints
// prevent the list adapter from silently skipping original preview operations.
struct NativePropertyWindowListServices {
    std::function<void(PropertyWindowState&,bool)> item_panel;
    std::function<void(PropertyWindowState&)> clear_details;
};
// Borrows a lifecycle-configured two-column list and a live inventory. Reset or
// destroy this binding before destroying its DLL layout or mutating its columns.
class NativePropertyWindowList final {
public:
    NativePropertyWindowList(HMODULE,HMODULE,PropertyWindowState&,NativeInventorySession&,
        NativePropertyWindowListServices);
    ~NativePropertyWindowList();
    NativePropertyWindowList(const NativePropertyWindowList&)=delete;
    NativePropertyWindowList& operator=(const NativePropertyWindowList&)=delete;
    void Populate(std::uint32_t category,bool preserve_scroll=false);
    void Clear();
    std::size_t OwnedItemCount() const noexcept;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
