#pragma once
#include "pal4/native_property_window.h"
#include "pal4/native_property_window_selection.h"
#include <memory>
namespace pal4 {
class NativeGameDatabase;
class NativeSceneRecordCatalog;
class NativePlayerSkillRuntime;
class NativeCommonDialogLayout;
class NativeMenuWindow;
struct PlayerManagerObjectState;
struct NativeSystemPropertyServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
    std::function<int()> can_start_action;
    std::function<std::pair<std::string,std::string>()> scene_key;
    std::function<void(std::string_view,std::uint32_t)> sound;
    std::function<void(PropertyWindowState&)> earthball;
    std::function<void(NativePropertyWindow&)> magic;
};
// Composes the persistent page and its menu/use/discard/role controllers.
// Borrowed world/components, UI, inventory, skills and dialog outlive Close.
class NativeSystemPropertyPage final {
public:
    NativeSystemPropertyPage(NativeCeguiUiSession&,const std::filesystem::path&,
        NativeInventorySession&,NativeUiCollectionObjects&,NativeEffectSceneSession&,
        NativePkFileManagerRuntime&,NativeEventHandlerArray&,std::uint32_t manager,
        PlayerManagerObjectState&,const NativeGameDatabase&,const NativeSceneRecordCatalog&,
        NativePlayerSkillRuntime&,NativeCommonDialogLayout&,NativeSystemPropertyServices);
    ~NativeSystemPropertyPage();
    NativeSystemPropertyPage(const NativeSystemPropertyPage&)=delete;
    NativeSystemPropertyPage& operator=(const NativeSystemPropertyPage&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    void DispatchCommand(std::string_view,std::uint32_t argument=0u);
    NativePropertyWindow& Page();
    NativeRoleSelector& Selector();
    NativeMenuWindow& Menu();
    void RethrowError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
