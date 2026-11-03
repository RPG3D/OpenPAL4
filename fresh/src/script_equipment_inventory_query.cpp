#include "pal4/script_equipment_inventory_query.h"

namespace pal4 {

bool ScriptEquipmentInventoryQuery::HasUsableEquipment(
    const std::int32_t id, IScriptEquipmentInventoryQueryRuntime& runtime) {
    if (!runtime.HasRecord(runtime.GameDatabase() + 0x58u, id)) return false;
    const auto record = runtime.FindRecord(runtime.GameDatabase() + 0x58u, id);
    if (record == 0u) return false;

    const auto category = runtime.ReadWord(record + 0x94u);
    const auto count = runtime.ElementCount(runtime.Inventory() + 0x7cu,
                                            category);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto item_category = runtime.ReadWord(record + 0x94u);
        const auto element = runtime.ElementAt(runtime.Inventory() + 0x7cu,
            item_category, static_cast<std::uint32_t>(index));
        const auto element_id = runtime.ReadWord(element + 0x90u);
        if (element_id == runtime.ReadWord(record + 0x90u) &&
            !runtime.CheckPlayersCondition(element))
            return true;
    }
    return false;
}

}  // namespace pal4
