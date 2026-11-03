#include "pal4/player_manager_slot_inventory_state.h"

namespace pal4 {

std::uint32_t PlayerManagerSlotInventoryRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotInventoryRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerSlotInventoryRuntimeStub::InventoryManager() {
    return 0u;
}
std::uint32_t PlayerManagerSlotInventoryRuntimeStub::FindItem(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint8_t PlayerManagerSlotInventoryRuntimeStub::CheckPlayerCondition(
    std::uint32_t) { return 0u; }

std::uint8_t PlayerManagerSlotInventoryState::Update(
    std::uint32_t object, std::uint32_t enabled,
    IPlayerManagerSlotInventoryRuntime& runtime) {
    auto result = static_cast<std::uint8_t>(enabled);
    if (enabled != 0u) {
        for (std::uint32_t index = 0u; index < 6u; ++index) {
            const auto slot = object + 0x920u + index * 4u;
            if (runtime.ReadDword(slot) == 0u) continue;
            const auto manager = runtime.InventoryManager();
            const auto key = runtime.ReadDword(slot);
            const auto item = runtime.FindItem(manager + 0x7cu, key, 7u);
            result = 0u;
            if (item != 0u)
                result = runtime.CheckPlayerCondition(item);
            if (item == 0u || result != 0u)
                runtime.WriteDword(slot, 0u);
        }
    }
    runtime.WriteDword(object + 0xb08u, enabled);
    return result;
}

}  // namespace pal4
