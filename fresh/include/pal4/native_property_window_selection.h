#pragma once
#include "pal4/property_window_selection.h"
#include <functional>
#include <memory>
#include <utility>
namespace pal4 {
class NativeCeguiUiSession;
class NativeEventHandlerArray;
class NativeGameDatabase;
class NativeSceneRecordCatalog;
class NativeInventorySession;
class NativeMenuWindow;
struct NativePropertyWindowSelectionServices {
    std::function<int()> can_start_action;
    // Original arena +8 / +224: Scene.SCENE / Scene.SECTION respectively.
    std::function<std::pair<std::string,std::string>()> scene_key;
    std::function<void(std::string_view,std::uint32_t)> command;
};
// Live inventory -> existing choice algorithm -> actual DLL popup. All borrowed
// services outlive Close. Commands re-resolve the selected inventory sequence.
class NativePropertyWindowSelection final {
public:
    NativePropertyWindowSelection(NativeCeguiUiSession&,NativeEventHandlerArray&,
        NativeInventorySession&,const NativeGameDatabase&,const NativeSceneRecordCatalog&,
        NativePropertyWindowSelectionServices);
    ~NativePropertyWindowSelection();
    NativePropertyWindowSelection(const NativePropertyWindowSelection&)=delete;
    NativePropertyWindowSelection& operator=(const NativePropertyWindowSelection&)=delete;
    void Show(PropertyWindowState&,std::uint32_t event_data);
    NativeMenuWindow& Menu();
    void RethrowEventError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
