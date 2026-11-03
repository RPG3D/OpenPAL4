#include "pal4/combat_data_structure.h"

#include <new>
#include <stdexcept>

namespace pal4 {

std::uint32_t CombatDataStructure::CleanupEventCollection(
    const std::uint32_t collection, ICombatDataStructureRuntime& runtime) {
    const auto sentinel = runtime.ReadWord(collection + 4u);
    if (sentinel == 0u) throw std::invalid_argument("null combat event sentinel");
    auto node = runtime.ReadWord(sentinel);
    while (node != sentinel) {
        const auto next = runtime.ReadWord(node);
        const auto previous = runtime.ReadWord(node + 4u);
        runtime.WriteWord(previous, next);
        runtime.WriteWord(next + 4u, previous);
        runtime.CleanupEventData(node + 8u);
        static_cast<void>(runtime.Free(node));
        runtime.WriteWord(collection + 8u,
                          runtime.ReadWord(collection + 8u) - 1u);
        node = next;
    }
    static_cast<void>(runtime.Free(sentinel));
    runtime.WriteWord(collection + 4u, 0u);
    runtime.WriteWord(collection + 8u, 0u);
    return 0u;
}

std::uint32_t CombatDataStructure::Construct(
    const std::uint32_t first, const std::uint32_t second,
    ICombatDataStructureRuntime& runtime) {
    const auto object = runtime.Allocate(0x24u);
    if (object == 0u) throw std::bad_alloc();
    runtime.WriteWord(object, first != 0u ? first : object);
    runtime.WriteWord(object + 4u, second != 0u ? second : object);
    return object;
}

std::uint32_t CombatDataStructure::DestroyRange(
    const std::uint32_t begin, const std::uint32_t end,
    ICombatDataStructureRuntime& runtime) {
    std::uint32_t result = 0u;
    for (auto entry = begin; entry != end; entry += 8u) {
        if (runtime.ReadByte(entry) == 0u) continue;
        const auto object = runtime.ReadWord(entry + 4u);
        if (object != 0u)
            result = runtime.DestroyVirtual(object, 1u);
    }
    return result;
}

}  // namespace pal4
