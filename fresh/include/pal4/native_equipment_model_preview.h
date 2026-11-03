#pragma once
#include "pal4/equipment_window.h"
#include <memory>
namespace pal4 {
class NativeInventorySession;
class NativeUiCollectionObjects;
class NativeEffectSceneSession;
class NativePkFileManagerRuntime;
// Owns the page's item/pedestal/effect IDs, borrowing their resource owners.
// Retire before the UI collection, effects, inventory and package manager.
class NativeEquipmentModelPreview final {
public:
    NativeEquipmentModelPreview(EquipmentWindowState&,NativeInventorySession&,
        NativeUiCollectionObjects&,NativeEffectSceneSession&,NativePkFileManagerRuntime&);
    ~NativeEquipmentModelPreview();
    NativeEquipmentModelPreview(const NativeEquipmentModelPreview&)=delete;
    NativeEquipmentModelPreview& operator=(const NativeEquipmentModelPreview&)=delete;
    void Update(std::uint32_t inventory_record);
    void SetVisible(bool);
    // Equipment details451D70 clears the current item independently of the
    // pedestal/effect. This endpoint returns the original-style remove result.
    std::uint32_t RemoveItem(std::uint32_t id);
    void Close();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
