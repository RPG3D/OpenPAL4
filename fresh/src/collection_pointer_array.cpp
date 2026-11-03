#include "pal4/collection_pointer_array.h"

#include <new>

namespace pal4 {

std::uint32_t CollectionPointerArrayRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
void CollectionPointerArrayRuntimeStub::WriteWord(
    std::uint32_t, std::uint32_t) {}
std::uint32_t CollectionPointerArrayRuntimeStub::AllocateMemory(
    std::uint32_t) { return 0u; }
void CollectionPointerArrayRuntimeStub::FreeMemory(std::uint32_t) {}

std::uint32_t CollectionPointerArray::PushBack(
    DwordCollectionSpanState& collection, const std::uint32_t value,
    ICollectionPointerArrayRuntime& runtime) {
    if ((collection.capacity - collection.end) / 4u >= 1u) {
        runtime.WriteWord(collection.end, value);
        collection.end += 4u;
        return collection.end;
    }

    const auto size = collection.begin == 0u ? 0u :
        (collection.end - collection.begin) / 4u;
    const auto growth = size > 1u ? size : 1u;
    const auto new_capacity = size + growth;
    const auto memory = runtime.AllocateMemory(new_capacity * 4u);
    if (memory == 0u) throw std::bad_alloc{};
    for (std::uint32_t index = 0u; index < size; ++index) {
        runtime.WriteWord(memory + index * 4u,
                          runtime.ReadWord(collection.begin + index * 4u));
    }
    runtime.WriteWord(memory + size * 4u, value);
    runtime.FreeMemory(collection.begin);
    collection.capacity = memory + new_capacity * 4u;
    collection.begin = memory;
    collection.end = memory + (size + 1u) * 4u;
    return collection.end;
}

}  // namespace pal4
