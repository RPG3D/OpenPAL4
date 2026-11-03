#pragma once
#include "pal4/equipment_window.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeInventorySession;
class NativeGameDatabase;
class NativePlayerScriptRefreshRuntime;
struct PlayerManagerObjectState;
// Borrows lifecycle controls and live game services. Update only previews;
// the shared algorithm restores the equipment slot and recomputes live stats.
class NativeEquipmentItemDetails final {
public:
    NativeEquipmentItemDetails(EquipmentWindowState&,NativeInventorySession&,PlayerManagerObjectState&,
        const NativeGameDatabase&,NativePlayerScriptRefreshRuntime&,std::function<std::uint32_t(std::uint32_t)> remove_preview);
    ~NativeEquipmentItemDetails();
    NativeEquipmentItemDetails(const NativeEquipmentItemDetails&)=delete;
    NativeEquipmentItemDetails& operator=(const NativeEquipmentItemDetails&)=delete;
    void Update(std::uint32_t inventory_record);
    std::uint32_t Clear();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
