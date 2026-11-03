#pragma once
#include "pal4/player_manager_slot_equipment.h"
#include <string>

namespace pal4 {
class NativeInventorySession;
class IBaseObjectNamedWeaponRuntime;
struct PlayerManagerSlotState;

// Uses live inventory snapshots (including loaded-save overrides), not the
// immutable database catalog. Inventory, slot and weapon services are borrowed.
class NativePlayerEquipmentVisualRuntime final : private IPlayerManagerSlotEquipmentRuntime {
public:
    NativePlayerEquipmentVisualRuntime(NativeInventorySession&,IBaseObjectNamedWeaponRuntime&);
    void Apply(PlayerManagerSlotState&);
    void Cleanup(PlayerManagerSlotState&);
private:
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint32_t InventoryManager() override;
    std::uint32_t FindItem(std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t CreateName(std::uint32_t) override;
    bool NameEquals(std::uint32_t,const char*) override;
    std::uint32_t CleanupName(std::uint32_t) override;
    std::uint32_t EmptyText() override;
    std::uint32_t SharedEffect() override;
    std::uint32_t IndexedText(std::uint32_t,std::uint32_t) override;
    void ApplyVisuals(std::uint32_t,const PlayerEquipmentVisuals&) override;
    void ApplyWeapon(std::uint32_t,const PlayerEquipmentVisuals&) override;
    NativeInventorySession& inventory_;
    IBaseObjectNamedWeaponRuntime& weapons_;
    std::string name_;
    bool name_live_{};
};
}
