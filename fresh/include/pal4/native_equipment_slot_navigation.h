#pragma once
#include "pal4/equipment_window.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeEventHandlerArray;
// Original secondary member+118: selection, arrows/Tab/Esc/Enter and slot hover.
// Borrows live layout/activity; mouse activation uses the actual DLL input path.
class NativeEquipmentSlotNavigation final {
public:
    NativeEquipmentSlotNavigation(EquipmentWindowState&,NativeEventHandlerArray&,
        std::function<void(bool)> panels_visible);
    ~NativeEquipmentSlotNavigation();
    NativeEquipmentSlotNavigation(const NativeEquipmentSlotNavigation&)=delete;
    NativeEquipmentSlotNavigation& operator=(const NativeEquipmentSlotNavigation&)=delete;
    bool HandleKey(std::uint32_t event);
    bool HandleMouseMove(std::uint32_t event);
    bool SelectFirstEnabled();
    bool ClearHighlight();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
