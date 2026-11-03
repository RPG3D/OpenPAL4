#pragma once
#include "pal4/equipment_window.h"
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;class NativeInventorySession;class NativeGameDatabase;
class NativeUiCollectionObjects;class NativeEffectSceneSession;class NativePkFileManagerRuntime;
class NativePlayerScriptRefreshRuntime;class NativeEventHandlerArray;class NativeCommonDialogLayout;
class NativeMenuWindow;struct PlayerManagerObjectState;
struct NativeEquipmentWindowServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
    std::function<void(std::string_view,std::uint32_t)> sound;
};
// Complete page owner; borrows the game's UI/world/inventory/player services.
// Global activity dispatch must route the owned members through this facade.
class NativeEquipmentWindow final {
public:
    NativeEquipmentWindow(NativeCeguiUiSession&,const std::filesystem::path&,NativeInventorySession&,
        PlayerManagerObjectState&,const NativeGameDatabase&,NativePlayerScriptRefreshRuntime&,
        NativeUiCollectionObjects&,NativeEffectSceneSession&,NativePkFileManagerRuntime&,
        NativeEventHandlerArray&,std::uint32_t manager,NativeCommonDialogLayout&,NativeEquipmentWindowServices);
    ~NativeEquipmentWindow();
    NativeEquipmentWindow(const NativeEquipmentWindow&)=delete;
    NativeEquipmentWindow& operator=(const NativeEquipmentWindow&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    std::uint32_t FrameAddress()const noexcept;
    const EquipmentWindowState& State()const;
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    void DispatchCommand(std::string_view,std::uint32_t sequence);
    NativeMenuWindow& Menu();
    void RethrowEventError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
