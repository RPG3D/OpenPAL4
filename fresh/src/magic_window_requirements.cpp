#include "pal4/magic_window_requirements.h"

namespace pal4 {

bool MagicWindowRequirements::Check(
    const MagicWindowState& state, const MagicDbStuntRecord& record,
    IMagicWindowRequirementsRuntime& runtime) {
    const auto player = runtime.GetPlayer(state.selected_character_index);
    if (player == 0 || record.kind != 2u)
        return false;
    if (runtime.ConditionContains(player, record.key))
        return false;
    if (!runtime.SortedContains(player, record.key))
        return false;
    if (record.required_stat > runtime.PlayerValue(player, 8u))
        return false;
    if (record.condition_key != 0u &&
        !runtime.HasCondition(player, record.condition_key))
        return false;

    if (record.special_item_id != 0u) {
        const auto special = runtime.LookupItem(record.special_item_id);
        if (special && !runtime.InventoryCount(*special))
            return false;
    }

    for (std::size_t index = 0; index < record.item_ids.size(); ++index) {
        const auto key = record.item_ids[index];
        if (key == 0u)
            continue;
        const auto item = runtime.LookupItem(key);
        if (!item)
            continue;
        const auto count = runtime.InventoryCount(*item).value_or(0);
        if (count < record.item_counts[index])
            return false;
    }
    return true;
}

}  // namespace pal4
