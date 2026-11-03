#include "pal4/collection_for_each.h"

#include <new>

namespace pal4 {

std::uint32_t CollectionForEachRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t CollectionForEachRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
void CollectionForEachRuntimeStub::WriteWord(std::uint32_t, std::uint32_t) {}
void CollectionForEachRuntimeStub::Free(std::uint32_t) {}
bool CollectionForEachRuntimeStub::AtEnd(std::uint32_t, std::uint32_t) {
    return true;
}
void CollectionForEachRuntimeStub::Advance(std::uint32_t, std::uint32_t) {}
void CollectionForEachRuntimeStub::Apply(
    std::uint32_t, const CollectionForEachVisitorState&, std::uint32_t) {}
std::uint32_t CollectionForEachRuntimeStub::Transform(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

namespace {

std::uint32_t Traverse(const std::uint32_t collection,
                       const std::uint32_t value,
                       const std::uint32_t parameter,
                       const std::uint32_t apply_slot,
                       ICollectionForEachRuntime& runtime) {
    std::uint32_t first = 0u;
    (void)CollectionIteratorLifecycle::CreateFromFirst(
        collection, first, runtime);
    if (first == 0u) throw std::bad_alloc{};
    try {
        while (!runtime.AtEnd(first, 24u)) {
            const auto element = CollectionIteratorLifecycle::GetValue(
                first, runtime);
            const CollectionForEachVisitorState visitor{
                0x849938u, parameter, value};
            runtime.Apply(element, visitor, apply_slot);
            runtime.Advance(first, 36u);
        }

        std::uint32_t third = 0u;
        (void)CollectionIteratorLifecycle::CreateFromThird(
            collection, third, runtime);
        if (third == 0u) throw std::bad_alloc{};
        try {
            while (!runtime.AtEnd(third, 24u)) {
                const auto element = CollectionIteratorLifecycle::GetValue(
                    third, runtime);
                const auto transformed = runtime.Transform(element, value, 0u);
                (void)Traverse(runtime.ReadWord(element + 4u), transformed,
                               parameter, apply_slot, runtime);
                runtime.Advance(third, 36u);
            }
        } catch (...) {
            (void)CollectionIteratorLifecycle::CleanupFirst(
                third, 1u, runtime);
            throw;
        }
        (void)CollectionIteratorLifecycle::CleanupFirst(third, 1u, runtime);
    } catch (...) {
        (void)CollectionIteratorLifecycle::CleanupSecond(first, 1u, runtime);
        throw;
    }
    return CollectionIteratorLifecycle::CleanupSecond(first, 1u, runtime);
}

}  // namespace

std::uint32_t CollectionForEach::Visit(
    const std::uint32_t collection, const std::uint32_t value,
    const std::uint32_t parameter, ICollectionForEachRuntime& runtime) {
    return Traverse(collection, value, parameter, 8u, runtime);
}

std::uint32_t CollectionForEach::SetProperties(
    const std::uint32_t collection, const std::uint32_t value,
    const std::uint32_t parameter, ICollectionForEachRuntime& runtime) {
    return Traverse(collection, value, parameter, 12u, runtime);
}

}  // namespace pal4
