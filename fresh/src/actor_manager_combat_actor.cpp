#include "pal4/actor_manager_combat_actor.h"

namespace pal4 {

std::uint32_t ActorManagerCombatActor::Construct(
    std::uint32_t object, std::uint32_t monster_record,
    IActorManagerCombatActorRuntime& runtime) {
    const auto damage_storage = runtime.Allocate(12u);
    const auto damage = damage_storage == 0u
        ? 0u : runtime.ConstructDamage(damage_storage);
    runtime.ConstructActorData(object, 2u, damage);
    runtime.InitializeArrays(object + 896u);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 716u, kFacetVtable);
    runtime.WriteWord(object + 724u, kPropertyVtable);
    (void)CopyFrom(object, monster_record, runtime);

    const auto facet_storage = runtime.Allocate(8u);
    const auto facet = facet_storage == 0u
        ? 0u : runtime.ConstructFacet(facet_storage, object);
    runtime.WriteWord(object + 1156u, facet);
    runtime.WriteByte(object + 1160u, 0u);
    return object;
}

std::uint32_t ActorManagerCombatActor::Value1148(
    std::uint32_t object, IActorManagerCombatActorRuntime& runtime) {
    return runtime.ReadWord(object + 1148u);
}

std::uint32_t ActorManagerCombatActor::Value1152(
    std::uint32_t object, IActorManagerCombatActorRuntime& runtime) {
    return runtime.ReadWord(object + 1152u);
}

bool ActorManagerCombatActor::HasPositive1144(
    std::uint32_t object, IActorManagerCombatActorRuntime& runtime) {
    return static_cast<std::int32_t>(runtime.ReadWord(object + 1144u)) > 0;
}

std::uint32_t ActorManagerCombatActor::Value960(
    std::uint32_t object, IActorManagerCombatActorRuntime& runtime) {
    return runtime.ReadWord(object + 960u);
}

std::uint32_t ActorManagerCombatActor::Delete(
    std::uint32_t object, std::uint8_t flags, std::uint32_t context,
    IActorManagerCombatActorRuntime& runtime) {
    Destroy(object, context, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

void ActorManagerCombatActor::Destroy(
    std::uint32_t object, std::uint32_t context,
    IActorManagerCombatActorRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 716u, kFacetVtable);
    runtime.WriteWord(object + 724u, kPropertyVtable);
    runtime.CleanupProperty(object, context);
    const auto facet = runtime.ReadWord(object + 1156u);
    if (facet != 0u) {
        runtime.DestroyFacet(facet);
        runtime.Free(facet);
    }
    runtime.DestroyTextArray(object + 1064u, 5u, 16u);
    runtime.DestroyTextArray(object + 968u, 6u, 16u);
    runtime.DestroyActorData(object, context);
}

}  // namespace pal4
