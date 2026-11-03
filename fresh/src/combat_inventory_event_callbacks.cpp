#include "pal4/combat_inventory_event_callbacks.h"

namespace pal4 {

std::uint32_t CombatInventoryEventCallbacks::UpdateCounter(
    const std::uint32_t event, ICombatInventoryEventRuntime& runtime) {
    const auto inventory = runtime.InventoryInstance();
    return runtime.UpdateAndClampCounter(inventory,
                                         runtime.ReadWord(event));
}

std::uint32_t CombatInventoryEventCallbacks::ProcessItem(
    const std::uint32_t event, ICombatInventoryEventRuntime& runtime) {
    const auto database = runtime.GameDatabase();
    runtime.FindProperty(database, runtime.ReadWord(event));
    const auto inventory = runtime.InventoryInstance();
    const auto count = runtime.ReadWord(event + 4u);
    const auto item = runtime.ReadWord(event);
    return runtime.ProcessItem(inventory, item, count);
}

std::int32_t CombatInventoryEventCallbacks::CreateMonsters(
    const std::uint32_t event, ICombatInventoryEventRuntime& runtime) {
    std::int32_t count = static_cast<std::int32_t>(
        runtime.ReadWord(event + 4u));
    std::int32_t index = 0;
    while (index < count) {
        const auto manager = runtime.ActorManager();
        const auto monster = runtime.ReadWord(event);
        runtime.CreateMonster(manager, monster, 6u, 0u);
        count = static_cast<std::int32_t>(
            runtime.ReadWord(event + 4u));
        ++index;
    }
    return count;
}

}  // namespace pal4
