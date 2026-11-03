#pragma once
#include "pal4/equipment_window.h"
#include "pal4/menu_window_visibility.h"
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;
class NativeEventHandlerArray;
class NativeInventorySession;
class NativeGameDatabase;
class NativeMenuWindow;
struct PlayerManagerObjectState;
// Borrows the live page, inventory and players. The command receives an
// equipment instance sequence, never a database ID or captured record pointer.
class NativeEquipmentWindowSelection final {
public:
    using Command=std::function<void(std::string_view,std::uint32_t)>;
    NativeEquipmentWindowSelection(NativeCeguiUiSession&,NativeEventHandlerArray&,
        EquipmentWindowState&,NativeInventorySession&,PlayerManagerObjectState&,
        const NativeGameDatabase&,Command);
    ~NativeEquipmentWindowSelection();
    NativeEquipmentWindowSelection(const NativeEquipmentWindowSelection&)=delete;
    NativeEquipmentWindowSelection& operator=(const NativeEquipmentWindowSelection&)=delete;
    bool Show(MenuWindowPoint);
    NativeMenuWindow& Menu();
    void RethrowEventError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
