#include "pal4/effect_system_object_list.h"

namespace pal4 {

std::uint32_t EffectSystemObjectList::InsertBefore(
    std::uint32_t list, std::uint32_t before,
    std::uint32_t object, IEffectSystemObjectListRuntime& runtime) {
    auto previous = runtime.ReadWord(before + 4u);
    const auto node = runtime.Allocate(12u);
    runtime.WriteWord(node, before);
    if (previous == 0u) previous = node;
    runtime.WriteWord(node + 4u, previous);
    runtime.WriteWord(before + 4u, node);
    runtime.WriteWord(previous, node);
    runtime.WriteWord(node + 8u, object);
    runtime.WriteWord(list + 8u, runtime.ReadWord(list + 8u) + 1u);
    return node;
}

std::uint32_t EffectSystemObjectList::EraseRange(
    std::uint32_t list, std::uint32_t first,
    std::uint32_t last, IEffectSystemObjectListRuntime& runtime) {
    auto node = first;
    while (node != last) {
        const auto next = runtime.ReadWord(node);
        const auto previous = runtime.ReadWord(node + 4u);
        runtime.WriteWord(previous, next);
        runtime.WriteWord(next + 4u, previous);
        runtime.Free(node);
        runtime.WriteWord(list + 8u, runtime.ReadWord(list + 8u) - 1u);
        node = next;
    }
    return node;
}

std::uint32_t EffectSystemObjectList::CreateNode(
    std::uint32_t next, std::uint32_t previous,
    IEffectSystemObjectListRuntime& runtime) {
    const auto node = runtime.Allocate(12u);
    runtime.WriteWord(node, next != 0u ? next : node);
    runtime.WriteWord(node + 4u, previous != 0u ? previous : node);
    return node;
}

}  // namespace pal4
