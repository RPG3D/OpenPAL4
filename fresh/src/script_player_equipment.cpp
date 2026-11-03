#include "pal4/script_player_equipment.h"

namespace pal4 {

std::uint8_t ScriptPlayerEquipment::Add(
    std::int32_t player_id, std::uint32_t equipment_id,
    IScriptPlayerEquipmentRuntime& runtime) {
    if (player_id < 0) return static_cast<std::uint8_t>(player_id);
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_id);
    if (player == 0u) return 0u;
    if (runtime.IsEquipment(runtime.GetGameDatabase() + 0x58u,
                            equipment_id) == 0u)
        return 0u;

    const auto record = runtime.ProcessInventoryCollection(
        runtime.GetInventory() + 0x7cu, equipment_id);
    const auto slots = runtime.ReadEquipmentSlots(player + 0x920u);
    const auto slot = InventoryItemSlots::FindSlotOrClassify(
        slots, record, runtime);
    if (slot < 6u)
        runtime.WriteWord(player + 0x920u + slot * 4u,
                          runtime.ReadDword(record + 0x190u));
    return runtime.RefreshEquipmentState(player + 0x654u);
}

}  // namespace pal4
