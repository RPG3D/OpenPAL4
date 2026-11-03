#include "pal4/actor_manager_property_facet.h"

namespace pal4 {
namespace {

std::uint32_t Delete(const std::uint32_t object, const std::uint8_t flags,
                     const std::uint32_t vtable,
                     IActorManagerPropertyFacetRuntime& runtime) {
    runtime.WriteWord(object, vtable);
    if (flags & 1u) runtime.Free(object);
    return object;
}

}  // namespace

std::uint32_t ActorManagerPropertyFacet::Construct(
    const std::uint32_t object, const std::uint32_t first,
    const std::uint32_t second,
    IActorManagerPropertyFacetRuntime& runtime) {
    runtime.WriteWord(object, kBaseVtable);
    runtime.WriteWord(object + 8u, first);
    runtime.WriteWord(object + 12u, second);
    return object;
}

void ActorManagerPropertyFacet::MarkFirst(
    const std::uint32_t object,
    IActorManagerPropertyFacetRuntime& runtime) {
    runtime.WriteWord(object, kFirstVtable);
}

void ActorManagerPropertyFacet::MarkSecond(
    const std::uint32_t object,
    IActorManagerPropertyFacetRuntime& runtime) {
    runtime.WriteWord(object, kSecondVtable);
}

std::uint32_t ActorManagerPropertyFacet::DeleteFirst(
    const std::uint32_t object, const std::uint8_t flags,
    IActorManagerPropertyFacetRuntime& runtime) {
    MarkFirst(object, runtime);
    if (flags & 1u) runtime.Free(object);
    return object;
}

std::uint32_t ActorManagerPropertyFacet::DeleteSecond(
    const std::uint32_t object, const std::uint8_t flags,
    IActorManagerPropertyFacetRuntime& runtime) {
    MarkSecond(object, runtime);
    if (flags & 1u) runtime.Free(object);
    return object;
}

std::uint32_t ActorManagerPropertyFacet::DeleteFirstInline(
    const std::uint32_t object, const std::uint8_t flags,
    IActorManagerPropertyFacetRuntime& runtime) {
    return Delete(object, flags, kFirstVtable, runtime);
}

std::uint32_t ActorManagerPropertyFacet::DeleteSecondInline(
    const std::uint32_t object, const std::uint8_t flags,
    IActorManagerPropertyFacetRuntime& runtime) {
    return Delete(object, flags, kSecondVtable, runtime);
}

}  // namespace pal4
