#pragma once
#include "pal4/equipment_window.h"
#include <memory>
namespace pal4 {
class NativeInventorySession;
struct PlayerManagerObjectState;
// Borrows original controls, inventory and player slots. No layout/callback/model
// ownership; all borrowed objects must outlive this adapter.
class NativeEquipmentCharacterDisplay final {
public:
    NativeEquipmentCharacterDisplay(EquipmentWindowState&,NativeInventorySession&,PlayerManagerObjectState&);
    ~NativeEquipmentCharacterDisplay();
    NativeEquipmentCharacterDisplay(const NativeEquipmentCharacterDisplay&)=delete;
    NativeEquipmentCharacterDisplay& operator=(const NativeEquipmentCharacterDisplay&)=delete;
    void SetCharacterImage(std::uint32_t character);
    void UpdateEquipment(std::uint32_t character);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
