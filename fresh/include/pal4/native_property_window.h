#pragma once
#include "pal4/property_window_lifecycle.h"
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
class NativeInventorySession;
class NativeUiCollectionObjects;
class NativeEffectSceneSession;
class NativePkFileManagerRuntime;
class NativeEventHandlerArray;
class NativeRoleSelector;
struct NativePropertyWindowServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
    // Required synchronous action endpoints. The page owns selection/command
    // routing, while gameplay and action-dialog owners supply their behavior.
    std::function<void(PropertyWindowState&,std::uint32_t)> selection;
    std::function<void(std::string_view,PropertyWindowState&,std::uint32_t)> command;
};
// Owns original DLL layout, typed activities, subscriptions, list and previews.
// All borrowed services must outlive Close. It does not enter System itself.
class NativePropertyWindow final {
public:
    NativePropertyWindow(NativeCeguiUiSession&,const std::filesystem::path&,NativeInventorySession&,
        NativeUiCollectionObjects&,NativeEffectSceneSession&,NativePkFileManagerRuntime&,
        NativeEventHandlerArray&,std::uint32_t manager,NativePropertyWindowServices);
    ~NativePropertyWindow();
    NativePropertyWindow(const NativePropertyWindow&)=delete;
    NativePropertyWindow& operator=(const NativePropertyWindow&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    std::uint32_t FrameAddress() const noexcept;
    const PropertyWindowState& State() const noexcept;
    bool OwnsActivity(std::uint32_t) const noexcept;
    std::uint32_t DispatchActiveCallback(std::uint32_t,std::uint32_t);
    std::uint8_t DispatchActivePredicate(std::uint32_t,std::uint32_t,std::uint32_t);
    void DispatchCommand(std::string_view,std::uint32_t argument);
    void RefreshItems(std::uint32_t category,bool preserve_scroll=false);
    // Original3182 transition and its after_use_property return callback.
    void UseMagicItem();
    void AfterUseMagicItem(NativeRoleSelector&);
    void RethrowEventError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
