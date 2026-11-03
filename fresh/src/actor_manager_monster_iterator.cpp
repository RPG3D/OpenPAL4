#include "pal4/actor_manager_monster_iterator.h"

namespace pal4 {

void ActorManagerMonsterIterator::MarkBase(
    const std::uint32_t iterator,
    IActorManagerMonsterIteratorRuntime& runtime) {
    runtime.WriteWord(iterator, kBaseVtable);
}

std::uint32_t ActorManagerMonsterIterator::Clone(
    const std::uint32_t source,
    IActorManagerMonsterIteratorRuntime& runtime) {
    const auto iterator = runtime.Allocate(12u);
    if (!iterator) return 0u;
    MarkBase(iterator, runtime);
    runtime.WriteWord(iterator + 4u, runtime.ReadWord(source + 4u));
    runtime.WriteWord(iterator + 8u, runtime.ReadWord(source + 8u));
    runtime.WriteWord(iterator, kDerivedVtable);
    return iterator;
}

std::uint32_t ActorManagerMonsterIterator::Delete(
    const std::uint32_t iterator, const std::uint8_t flags,
    IActorManagerMonsterIteratorRuntime& runtime) {
    MarkBase(iterator, runtime);
    if (flags & 1u) runtime.Free(iterator);
    return iterator;
}

}  // namespace pal4
