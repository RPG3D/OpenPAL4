#pragma once

#include "pal4/player_manager_slot_inventory_state.h"

namespace pal4 {
struct PlayerManagerObjectState;
class NativeBaseObjectMemory;
class NativeInventorySession;

// Equipment activation uses the same four persistent players and inventory as
// load preparation. Instance sequence IDs, not database item IDs, identify gear.
class NativePlayerInventoryStateRuntime final : public IPlayerManagerSlotInventoryRuntime {
public:
    NativePlayerInventoryStateRuntime(PlayerManagerObjectState& players,
        NativeBaseObjectMemory& memory, NativeInventorySession& inventory)
        : players_(players), memory_(memory), inventory_(inventory) {}
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t InventoryManager() override;
    std::uint32_t FindItem(std::uint32_t collection, std::uint32_t key,
                           std::uint32_t category) override;
    std::uint8_t CheckPlayerCondition(std::uint32_t item) override;
    //4C4260: inactive party members' equipped instances are hidden from sale.
    bool NoInactivePlayerHasItem(std::uint32_t item);
private:
    PlayerManagerObjectState& players_;
    NativeBaseObjectMemory& memory_;
    NativeInventorySession& inventory_;
};
}  // namespace pal4
