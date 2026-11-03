#pragma once
#include "pal4/equipment_window.h"
#include <memory>
#include <cstddef>
namespace pal4 {
class NativeInventorySession;
class NativeGameDatabase;
struct PlayerManagerObjectState;
// Borrows lifecycle-configured original list/arrows, inventory, players and DB.
// Owns both cells of each row; Close before destroying borrowed DLL controls.
class NativeEquipmentWindowList final {
public:
    NativeEquipmentWindowList(EquipmentWindowState&,NativeInventorySession&,PlayerManagerObjectState&,const NativeGameDatabase&);
    ~NativeEquipmentWindowList();
    NativeEquipmentWindowList(const NativeEquipmentWindowList&)=delete;
    NativeEquipmentWindowList& operator=(const NativeEquipmentWindowList&)=delete;
    void Populate(std::uint32_t category,std::uint32_t character);
    void Clear();
    // Original450580 already resets before4507E0. Reject nonempty callers so
    // composition does not reset twice and emit extra DLL hover events.
    void PopulateCleared(std::uint32_t category,std::uint32_t character);
    void Close();
    std::size_t OwnedItemCount()const noexcept;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
