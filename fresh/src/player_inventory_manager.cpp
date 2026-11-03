#include "pal4/player_inventory_manager.h"

namespace pal4 {

std::uint8_t PlayerInventoryManager::Manage(
    const std::uint32_t item, const std::uint32_t inventory_key,
    IPlayerInventoryRuntime& runtime) {
    const auto inventory = runtime.InventorySubsystem();
    const auto entry = runtime.FindInventoryEntry(
        inventory + 0x7cu, inventory_key, 7u);

    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto manager = runtime.PlayerManager();
        const auto player = runtime.GetPlayer(manager, index);
        if (player == 0u || !runtime.Contains(player + 0x920u, item))
            continue;

        const auto slot = runtime.FindSlot(player + 0x920u, entry);
        if (slot >= 0 && slot < 6) {
            const auto value = runtime.ReadWord(entry + 0x190u);
            runtime.WriteWord(player + 0x920u +
                                  static_cast<std::uint32_t>(slot) * 4u,
                              value);
        }
        return static_cast<std::uint8_t>(
            runtime.Refresh(player + 0x654u, 0x114u));
    }
    return 0u;
}

}  // namespace pal4
