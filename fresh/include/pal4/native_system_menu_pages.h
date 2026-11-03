#pragma once
#include "pal4/native_system_property_page.h"
#include "pal4/native_system_magic_page.h"
#include "pal4/native_equipment_window.h"
#include "pal4/native_save_window.h"
#include "pal4/native_system_menu_decorations.h"
#include "pal4/native_role_state_window.h"
#include "pal4/native_mission_window.h"
#include "pal4/native_smith_window.h"
#include <memory>
namespace pal4 {
class NativeArenaLoadRuntime;
class NativePalGameIVTextureStorage;
class NativePalGameIVStateList;
class NativeEventHandlerArray;
class NativeInventorySession;
class InputManager;
class NativeSystemToolbarActions;
// Native window/controller lifetime shared by persistent and bounded System use.
// All borrowed objects and callbacks must outlive this owner. Close only after
// leaving System, before UI/DLL shutdown or retiring the global activity array.
struct NativeSystemMenuPageServices {
    NativeSystemMenuDecorationServices decorations;
    NativeRoleStateWindowServices role;
    std::function<void(std::uint32_t)> tutorial;
    // Missing factories reject show. Hiding an absent page follows4AA000.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> page;
    // Factory is invoked only on show, after the System world exists.
    std::function<std::unique_ptr<NativeSystemPropertyPage>()> property;
    std::function<std::unique_ptr<NativeSystemMagicPage>()> magic;
    std::function<std::unique_ptr<NativeEquipmentWindow>()> equipment;
    std::function<std::unique_ptr<NativeSaveWindow>()> save;
    std::function<std::unique_ptr<NativeSmithWindow>()> smith;
    // Optional in System-only hosts; ordinary Foundry close requires both.
    std::uint32_t trade{};
    std::function<std::uint8_t(std::uint32_t)> clear_trade;
};
class NativeSystemMenuPages final {
public:
    NativeSystemMenuPages(NativeCeguiUiSession&,const std::filesystem::path&,
        NativeArenaLoadRuntime&,NativePalGameIVTextureStorage&,NativePalGameIVStateList&,
        PlayerManagerObjectState&,const NativeGameDatabase&,NativeEventHandlerArray&,
        InputManager&,NativeInventorySession&,std::uint32_t manager,NativeSystemMenuPageServices);
    ~NativeSystemMenuPages();
    NativeSystemMenuPages(const NativeSystemMenuPages&)=delete;
    NativeSystemMenuPages& operator=(const NativeSystemMenuPages&)=delete;
    void Event(std::uint32_t key,std::uint32_t first,std::uint32_t second);
    bool OwnsActivity(std::uint32_t) const noexcept;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    NativeSystemMenuDecorations& Decorations();
    NativeRoleStateWindow* Role() const noexcept;
    NativeMissionWindow* Mission() const noexcept;
    NativeSystemPropertyPage* Property() const noexcept;
    NativeSystemMagicPage* Magic() const noexcept;
    NativeEquipmentWindow* Equipment() const noexcept;
    NativeSaveWindow* Save() const noexcept;
    NativeSmithWindow* Smith() const noexcept;
    bool SmithActive() const noexcept;
    // Stable shared byte borrowed by NativeSmithStateRuntime until Close.
    std::uint8_t& SmithFlagStorage();
    void SetSmithActive(bool);
    std::uint32_t FindFrameRoot(std::string_view name) const;
    // Retire borrowed component/world services before releasing the UI world.
    void ReleaseProperty();
    void ReleaseWorldPages();
    NativeSystemToolbarActions& Toolbar();
    void RethrowError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
