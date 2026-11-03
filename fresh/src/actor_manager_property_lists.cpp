#include "pal4/actor_manager_property_lists.h"

#include <initializer_list>
#include <new>

namespace pal4 {

std::uint32_t ActorManagerPropertyLists::Initialize(
    std::uint32_t object, IActorManagerPropertyListsRuntime& runtime) {
    // IDA reads an uninitialized stack byte for these two flags.
    runtime.WriteByte(object, 0u);
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteByte(object + 16u, 0u);
    for (const auto offset : {20u, 24u, 28u})
        runtime.WriteWord(object + offset, 0u);

    const auto secondary = runtime.Allocate(96u);
    if (secondary == 0u) throw std::bad_alloc();
    runtime.Free(0u);
    runtime.WriteWord(object + 28u, secondary + 96u);
    runtime.WriteWord(object + 20u, secondary);
    runtime.WriteWord(object + 24u, secondary);

    const auto primary = runtime.Allocate(32u);
    if (primary == 0u) {
        runtime.Free(secondary);
        for (const auto offset : {20u, 24u, 28u})
            runtime.WriteWord(object + offset, 0u);
        throw std::bad_alloc();
    }
    runtime.Free(0u);
    runtime.WriteWord(object + 12u, primary + 32u);
    runtime.WriteWord(object + 4u, primary);
    runtime.WriteWord(object + 8u, primary);
    return object;
}

}  // namespace pal4
