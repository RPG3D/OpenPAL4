#include "pal4/crafting_application.h"

#include <map>

namespace pal4 {

std::int32_t CraftingApplication::Apply(
    const std::uint32_t object, const std::uint32_t recipe,
    const std::uint32_t selected_item,
    ICraftingApplicationRuntime& runtime) {
    runtime.PlayMusic("wa09.wav", 1u);
    const auto recipe_data = runtime.FindRecipe(runtime.GameDb() + 0x80u,
                                                recipe);

    // IDA's tree orders unsigned item IDs and keeps the first duplicate.
    std::map<std::uint32_t, std::uint32_t> remaining;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto item = runtime.ReadWord(recipe_data + 0x98u + index * 4u);
        const auto quantity = runtime.ReadWord(
            recipe_data + 0xa8u + index * 4u);
        if (static_cast<std::int32_t>(quantity) > 0)
            remaining.emplace(item, quantity);
    }

    for (auto current = remaining.begin(); current != remaining.end();) {
        const auto item = current->first;
        const auto quantity = current->second;
        if (runtime.IsDirectItem(runtime.GameDb(), item)) {
            runtime.FindOrLogDirectItem(runtime.GameDb(), item);
            runtime.RemoveDirectItem(runtime.Inventory(), item, quantity);
            current = remaining.erase(current);
            continue;
        }

        if (selected_item != 0xffffffffu) {
            const auto inventory = runtime.Inventory();
            const auto entry = runtime.FindInventoryEntry(
                inventory + 0x7cu, selected_item, 7u);
            if (entry != 0u &&
                runtime.ReadWord(entry + 0x90u) == item) {
                current = remaining.erase(current);
                continue;
            }
        }
        ++current;
    }

    if (!remaining.empty()) return -1;

    const auto product = runtime.ReadWord(recipe_data + 0x68u);
    const auto inventory = runtime.Inventory();
    const auto created = runtime.ProcessGameDataCollection(
        inventory + 0x7cu, product);
    if (selected_item != 0xffffffffu) {
        const auto selected_inventory = runtime.Inventory();
        const auto entry = runtime.FindInventoryEntry(
            selected_inventory + 0x7cu, selected_item, 7u);
        if (runtime.CheckPlayerCondition(entry)) {
            runtime.ManagePlayerItem(
                object, selected_item, runtime.ReadWord(created + 0x190u));
        }
        runtime.RemoveInventoryEntry(runtime.Inventory() + 0x7cu, entry);
    }
    if (runtime.ReadByte(object) == 0u)
        runtime.UpdateAndClampCounter(runtime.Inventory(), -80);
    return 0;
}

}  // namespace pal4
