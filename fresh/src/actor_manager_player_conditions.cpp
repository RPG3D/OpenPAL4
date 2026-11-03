#include "pal4/actor_manager_player_conditions.h"

namespace pal4 {

bool ActorManagerPlayerConditions::CheckPrimary(
    const std::uint32_t actor, const std::uint32_t requirement,
    IActorManagerPlayerConditionRuntime& runtime) {
    const auto values = actor + 920u;
    const auto key = runtime.ReadWord(requirement);
    if (runtime.AlreadyUsed(values, key) ||
        !runtime.FixedContains(values, key))
        return false;
    const auto level = runtime.ActorSlot52(actor, 8u);
    if (runtime.ReadSignedWord(requirement + 116u) > level)
        return false;

    const auto script = runtime.ReadWord(requirement + 128u);
    if (script && !runtime.ScriptElementExists(actor, script))
        return false;

    const auto inventory = runtime.Inventory();
    const auto required_resource = runtime.ReadSignedWord(
        requirement + 120u);
    const auto available_resource = runtime.ReadSignedWord(
        inventory + 308u);
    if (required_resource > available_resource)
        return false;

    const auto required_item = runtime.ReadWord(requirement + 124u);
    if (required_item) {
        const auto pair = runtime.GameDbPair(required_item);
        if (pair) {
            const auto id = runtime.ReadWord(pair);
            const auto variant = runtime.ReadWord(pair + 4u);
            if (!runtime.FindInventoryElement(runtime.Inventory(), id,
                                              variant))
                return false;
        }
    }

    for (std::uint32_t i = 0u; i < 5u; ++i) {
        const auto item_key = runtime.ReadWord(requirement + 72u + 4u * i);
        const auto quantity = runtime.ReadSignedWord(
            requirement + 92u + 4u * i);
        if (!item_key) continue;
        const auto pair = runtime.GameDbPair(item_key);
        if (!pair) continue;
        const auto id = runtime.ReadWord(pair);
        const auto variant = runtime.ReadWord(pair + 4u);
        const auto element = runtime.FindInventoryElement(
            runtime.Inventory(), id, variant);
        const auto available = element ?
            runtime.ReadSignedWord(element + 8u) : 0;
        if (available < quantity) return false;
    }
    return true;
}

bool ActorManagerPlayerConditions::CheckSecondary(
    const std::uint32_t actor, const std::uint32_t requirement,
    IActorManagerPlayerConditionRuntime& runtime) {
    const auto values = actor + 920u;
    const auto key = runtime.ReadWord(requirement);
    if (runtime.AlreadyUsed(values, key) ||
        !runtime.IndexedContains(values,
                                 runtime.ReadWord(requirement + 4u), key))
        return false;
    const auto level = runtime.ActorSlot52(actor, 9u);
    return runtime.ReadSignedWord(requirement + 76u) <= level;
}

}  // namespace pal4
