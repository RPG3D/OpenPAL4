#pragma once
#include "pal4/equipment_window.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeEquipmentWindowList;
class NativeEquipmentItemDetails;
class NativeEquipmentCharacterDisplay;
struct PlayerManagerObjectState;
// Borrows page components; combines original450580/4507B0 and panel visibility.
// The model service binds NativeEquipmentModelPreview::SetVisible in the page.
class NativeEquipmentWindowPresentation final {
public:
    NativeEquipmentWindowPresentation(EquipmentWindowState&,PlayerManagerObjectState&,
        NativeEquipmentWindowList&,NativeEquipmentItemDetails&,NativeEquipmentCharacterDisplay&,
        std::function<void(bool)> model_visible);
    ~NativeEquipmentWindowPresentation();
    NativeEquipmentWindowPresentation(const NativeEquipmentWindowPresentation&)=delete;
    NativeEquipmentWindowPresentation& operator=(const NativeEquipmentWindowPresentation&)=delete;
    void UpdateList(std::uint32_t category,std::uint32_t character);
    void UpdateDisplay(std::uint32_t record,std::uint32_t character);
    void SetPanelsVisible(bool);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
