#pragma once
#include "pal4/property_window_lifecycle.h"
#include <memory>
namespace pal4 {
class NativeInventorySession;
class NativeUiCollectionObjects;
class NativePkFileManagerRuntime;
// Synchronous model endpoint for NativePropertyWindowDetails. Borrowed state,
// inventory, component collection and package owner must outlive this adapter.
class NativePropertyWindowModelPreview final {
public:
    NativePropertyWindowModelPreview(PropertyWindowState&, NativeInventorySession&,
        NativeUiCollectionObjects&, NativePkFileManagerRuntime&);
    ~NativePropertyWindowModelPreview();
    NativePropertyWindowModelPreview(const NativePropertyWindowModelPreview&)=delete;
    NativePropertyWindowModelPreview& operator=(const NativePropertyWindowModelPreview&)=delete;
    std::uint32_t Update(PropertyWindowState&, std::uint32_t inventory_record);
    void Clear();
    void Close();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
