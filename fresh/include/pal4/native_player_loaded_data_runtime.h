#pragma once

#include "pal4/player_manager_operations.h"
#include "pal4/player_manager_slot_inventory_state.h"
#include "pal4/player_manager_slot_processor.h"

namespace pal4 {

// Connects the rewritten 0x441360 loop to the rewritten slot operations.
// The caller supplies the inventory and game-data services used by those
// operations; neither is replaced with a default no-op implementation.
class NativePlayerLoadedDataRuntime final :
    public PlayerManagerOperationsRuntimeStub {
public:
    NativePlayerLoadedDataRuntime(
        std::uint32_t manager,
        IPlayerManagerSlotInventoryRuntime& inventory,
        IPlayerManagerSlotProcessRuntime& processor)
        : manager_(manager), inventory_(inventory), processor_(processor) {}

    std::uint32_t UpdateInventory(std::uint32_t slot,
                                  std::uint32_t enabled) override;
    void ProcessGameData(std::uint32_t slot) override;
    void CallScriptIndex(std::uint32_t script,
                         std::uint32_t virtual_slot,
                         std::uint32_t index) override;

private:
    void RequireSlot(std::uint32_t slot) const;

    std::uint32_t manager_;
    IPlayerManagerSlotInventoryRuntime& inventory_;
    IPlayerManagerSlotProcessRuntime& processor_;
};

}  // namespace pal4
