#include "pal4/actor_manager_dynamic_effects.h"

namespace pal4 {

void ActorManagerDynamicEffects::RemoveMatching(
    std::uint32_t actor, std::uint32_t context, std::uint32_t record,
    IActorManagerDynamicEffectsRuntime& runtime) {
    if (record == 0u) return;
    const auto tree = actor + 836u;
    const auto key = static_cast<std::int32_t>(runtime.ReadWord(record));
    const auto candidate = runtime.LowerBound(tree, key);
    const auto head = runtime.ReadWord(actor + 840u);
    if (candidate == head ||
        key < static_cast<std::int32_t>(runtime.ReadWord(candidate + 12u)))
        return;
    runtime.NotifyEffectEnd(candidate + 16u, context);
    runtime.EraseNode(tree, candidate);
}

void ActorManagerDynamicEffects::Add(
    std::uint32_t actor, std::uint32_t context, std::uint32_t record,
    IActorManagerDynamicEffectsRuntime& runtime) {
    if (record == 0u) return;
    RemoveMatching(actor, context, record, runtime);

    const auto created = runtime.CreateEffect(actor, record + 68u,
                                              record + 84u);
    if (!runtime.IsValid(created)) {
        runtime.ResetTemporary(created);
        return;
    }

    const auto key = static_cast<std::int32_t>(runtime.ReadWord(record));
    const auto copy = runtime.CopyEffect(created);
    const auto tree = actor + 836u;
    const auto head = runtime.ReadWord(actor + 840u);
    const auto nil = runtime.NilNode();
    auto cursor = runtime.ReadWord(head + 4u);
    auto parent = head;
    bool less = true;
    while (cursor != nil) {
        parent = cursor;
        less = key < static_cast<std::int32_t>(runtime.ReadWord(cursor + 12u));
        cursor = runtime.ReadWord(cursor + (less ? 0u : 8u));
    }

    bool insert = runtime.ReadByte(actor + 844u) != 0u;
    auto existing = parent;
    if (!insert && less) {
        if (parent == runtime.FirstNode(tree)) {
            insert = true;
        } else {
            existing = runtime.Predecessor(parent);
        }
    }
    if (!insert &&
        static_cast<std::int32_t>(runtime.ReadWord(existing + 12u)) < key)
        insert = true;

    const auto node = insert
        ? runtime.InsertNode(tree, cursor, parent, key, copy)
        : existing;
    runtime.RecordResult(node, insert);
    runtime.ResetTemporary(copy);
    runtime.ResetTemporary(created);
}

}  // namespace pal4
