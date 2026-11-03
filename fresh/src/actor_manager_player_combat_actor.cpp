#include "pal4/actor_manager_player_combat_actor.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t ActorManagerPlayerCombatActor::Construct(
    const std::uint32_t actor, const std::uint32_t source,
    IActorManagerPlayerCombatActorRuntime& runtime) {
    const auto damage_storage = runtime.Allocate(12u);
    const auto damage = damage_storage
        ? runtime.ConstructDamage(damage_storage) : 0u;
    runtime.ConstructActorData(actor, 1u, damage);
    for (std::uint32_t offset = 896u; offset < 920u; offset += 4u)
        runtime.WriteWord(actor + offset, 0u);
    runtime.ConstructTextArray(actor + 920u, 5u);
    runtime.ConstructTextArray(actor + 1000u, 5u);

    const auto marker = static_cast<std::uint8_t>(source);
    runtime.WriteByte(actor + 1080u, marker);
    for (const auto offset : {1084u, 1088u, 1092u})
        runtime.WriteWord(actor + offset, 0u);
    runtime.WriteByte(actor + 1096u, marker);
    for (const auto offset : {1100u, 1104u, 1108u})
        runtime.WriteWord(actor + offset, 0u);
    runtime.ConstructEmbeddedArrays(actor + 1112u);
    runtime.WriteWord(actor + 1296u, 0u);
    runtime.WriteByte(actor + 1292u, marker);
    runtime.WriteWord(actor + 1300u, 0u);
    runtime.WriteWord(actor + 1304u, 0u);
    runtime.WriteByte(actor + 1312u, 0u);
    runtime.WriteWord(actor + 1316u, 0u);
    runtime.WriteWord(actor, kVtable);
    runtime.WriteWord(actor + 716u, kFacetVtable);
    runtime.WriteWord(actor + 724u, kPropertyVtable);
    runtime.CopyActorData(actor, source);
    (void)InitializeFacet(actor, runtime);
    return actor;
}

std::uint32_t ActorManagerPlayerCombatActor::Value1200(
    const std::uint32_t actor,
    IActorManagerPlayerCombatActorRuntime& runtime) {
    return runtime.ReadWord(actor + 1200u);
}

std::uint32_t ActorManagerPlayerCombatActor::Value1204(
    const std::uint32_t actor,
    IActorManagerPlayerCombatActorRuntime& runtime) {
    return runtime.ReadWord(actor + 1204u);
}

std::uint32_t ActorManagerPlayerCombatActor::Value1196(
    const std::uint32_t actor,
    IActorManagerPlayerCombatActorRuntime& runtime) {
    return runtime.ReadWord(actor + 1196u);
}

std::uint32_t ActorManagerPlayerCombatActor::InitializeFacet(
    const std::uint32_t actor,
    IActorManagerPlayerCombatActorRuntime& runtime) {
    const auto storage = runtime.Allocate(8u);
    const auto facet = storage ? runtime.ConstructFacet(storage, actor) : 0u;
    runtime.WriteWord(actor + 1288u, facet);
    runtime.WriteByte(actor + 1308u, 0u);
    return facet;
}

std::uint32_t ActorManagerPlayerCombatActor::Delete(
    const std::uint32_t actor, const std::uint8_t flags,
    const std::uint32_t context,
    IActorManagerPlayerCombatActorRuntime& runtime) {
    Destroy(actor, context, runtime);
    if (flags & 1u) runtime.Free(actor);
    return actor;
}

void ActorManagerPlayerCombatActor::Destroy(
    const std::uint32_t actor, const std::uint32_t context,
    IActorManagerPlayerCombatActorRuntime& runtime) {
    runtime.WriteWord(actor, kVtable);
    runtime.WriteWord(actor + 716u, kFacetVtable);
    runtime.WriteWord(actor + 724u, kPropertyVtable);
    runtime.CleanupProperty(actor, context);
    const auto facet = runtime.ReadWord(actor + 1288u);
    if (facet) {
        runtime.DestroyFacet(facet);
        runtime.Free(facet);
    }
    if (runtime.ReadByte(actor + 1312u)) {
        const auto child = runtime.ReadWord(actor + 1316u);
        if (child) runtime.DeleteChild(child, 1u);
    }
    runtime.Free(runtime.ReadWord(actor + 1296u));
    for (const auto offset : {1296u, 1300u, 1304u})
        runtime.WriteWord(actor + offset, 0u);
    runtime.DestroyTextArray(actor + 1208u, 5u);
    runtime.DestroyTextArray(actor + 1112u, 4u);
    runtime.Free(runtime.ReadWord(actor + 1100u));
    for (const auto offset : {1100u, 1104u, 1108u})
        runtime.WriteWord(actor + offset, 0u);
    runtime.Free(runtime.ReadWord(actor + 1084u));
    for (const auto offset : {1084u, 1088u, 1092u})
        runtime.WriteWord(actor + offset, 0u);
    runtime.DestroyTextArray(actor + 1000u, 5u);
    runtime.DestroyTextArray(actor + 920u, 5u);
    runtime.DestroyActorData(actor, context);
}

}  // namespace pal4
