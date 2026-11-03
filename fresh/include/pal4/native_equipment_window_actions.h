#pragma once
#include "pal4/equipment_window.h"
#include "pal4/native_common_dialog_layout.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeInventorySession;
class NativeGameDatabase;
class NativePlayerScriptRefreshRuntime;
struct PlayerManagerObjectState;
struct NativeEquipmentActionServices {
    NativeCommonDialogLayout::SoundPlayer sound;
    std::function<void(std::uint32_t category,std::uint32_t character)> refresh_list;
    std::function<void(std::uint32_t record,std::uint32_t character)> refresh_display;
};
// Original action primitives. The original menu's ownership/level gates must
// run before invoking Equip/ShowDiscard. Borrowed page/game/UI outlive Close.
class NativeEquipmentWindowActions final {
public:
    NativeEquipmentWindowActions(EquipmentWindowState&,NativeInventorySession&,PlayerManagerObjectState&,
        const NativeGameDatabase&,NativePlayerScriptRefreshRuntime&,NativeCommonDialogLayout&,NativeEquipmentActionServices);
    ~NativeEquipmentWindowActions();
    NativeEquipmentWindowActions(const NativeEquipmentWindowActions&)=delete;
    NativeEquipmentWindowActions& operator=(const NativeEquipmentWindowActions&)=delete;
    bool Equip(std::uint32_t sequence);
    bool ShowDiscard(std::uint32_t sequence);
    bool Unequip(std::uint32_t equipment_control);
    void CancelPending();
    void Close();
private:
    class Runtime;
    std::shared_ptr<Runtime> runtime_;
};
}
