#pragma once
#include "pal4/property_window_lifecycle.h"
#include <windows.h>
#include <memory>
namespace pal4 {
class NativeUiCollectionObjects;
class NativeEffectSceneSession;
class NativePkFileManagerRuntime;
// Borrows original DLL controls, collection, bound UI effect group and packages.
// Close before retiring any borrowed owner or its page layout.
class NativePropertyWindowPanelPreview final {
public:
    NativePropertyWindowPanelPreview(HMODULE,PropertyWindowState&,NativeUiCollectionObjects&,
        NativeEffectSceneSession&,NativePkFileManagerRuntime&);
    ~NativePropertyWindowPanelPreview();
    NativePropertyWindowPanelPreview(const NativePropertyWindowPanelPreview&)=delete;
    NativePropertyWindowPanelPreview& operator=(const NativePropertyWindowPanelPreview&)=delete;
    void SetItemPanel(bool visible);
    void ToggleModel(bool visible);
    void Close();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
