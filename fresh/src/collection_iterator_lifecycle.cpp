#include "pal4/collection_iterator_lifecycle.h"

namespace pal4 {

std::uint32_t CollectionIteratorLifecycleRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t CollectionIteratorLifecycleRuntimeStub::ReadWord(std::uint32_t) {
    return 0u;
}
void CollectionIteratorLifecycleRuntimeStub::WriteWord(std::uint32_t,
                                                        std::uint32_t) {}
void CollectionIteratorLifecycleRuntimeStub::Free(std::uint32_t) {}

std::uint32_t& CollectionIteratorLifecycle::CreateFromThird(
    const std::uint32_t group, std::uint32_t& output,
    ICollectionIteratorLifecycleRuntime& runtime) {
    const CollectionIteratorState temporary{
        0x8497d8u, runtime.ReadWord(group + 36u), group + 32u};
    output = CloneFirst(temporary, runtime);
    return output;
}

std::uint32_t& CollectionIteratorLifecycle::CreateFromFirst(
    const std::uint32_t group, std::uint32_t& output,
    ICollectionIteratorLifecycleRuntime& runtime) {
    const CollectionIteratorState temporary{
        0x849804u, runtime.ReadWord(group + 4u), group};
    output = CloneSecond(temporary, runtime);
    return output;
}

void CollectionIteratorLifecycle::InitFirstBase(
    const std::uint32_t iterator, ICollectionIteratorLifecycleRuntime& runtime) {
    runtime.WriteWord(iterator, 0x849830u);
}

void CollectionIteratorLifecycle::InitSecondBase(
    const std::uint32_t iterator, ICollectionIteratorLifecycleRuntime& runtime) {
    runtime.WriteWord(iterator, 0x84985cu);
}

std::uint32_t CollectionIteratorLifecycle::CloneFirst(
    const CollectionIteratorState& source,
    ICollectionIteratorLifecycleRuntime& runtime) {
    const auto result = runtime.Allocate(12u);
    if (result == 0u) return 0u;
    InitFirstBase(result, runtime);
    runtime.WriteWord(result + 4u, source.current);
    runtime.WriteWord(result + 8u, source.owner);
    runtime.WriteWord(result, 0x8497d8u);
    return result;
}

std::uint32_t CollectionIteratorLifecycle::CloneSecond(
    const CollectionIteratorState& source,
    ICollectionIteratorLifecycleRuntime& runtime) {
    const auto result = runtime.Allocate(12u);
    if (result == 0u) return 0u;
    InitSecondBase(result, runtime);
    runtime.WriteWord(result + 4u, source.current);
    runtime.WriteWord(result + 8u, source.owner);
    runtime.WriteWord(result, 0x849804u);
    return result;
}

std::uint32_t CollectionIteratorLifecycle::GetValue(
    const std::uint32_t iterator,
    ICollectionIteratorLifecycleRuntime& runtime) {
    return runtime.ReadWord(runtime.ReadWord(iterator + 4u));
}

std::uint32_t CollectionIteratorLifecycle::NextFromFirst(
    const std::uint32_t iterator,
    ICollectionIteratorLifecycleRuntime& runtime) {
    const auto owner = runtime.ReadWord(iterator + 8u);
    runtime.WriteWord(iterator + 4u, runtime.ReadWord(owner + 4u));
    return owner;
}

std::uint32_t CollectionIteratorLifecycle::NextFromSecond(
    const std::uint32_t iterator,
    ICollectionIteratorLifecycleRuntime& runtime) {
    const auto owner = runtime.ReadWord(iterator + 8u);
    runtime.WriteWord(iterator + 4u, runtime.ReadWord(owner + 8u));
    return owner;
}

std::uint32_t CollectionIteratorLifecycle::DestroySecond(
    const std::uint32_t iterator, const std::uint32_t flag,
    ICollectionIteratorLifecycleRuntime& runtime) {
    InitSecondBase(iterator, runtime);
    if ((flag & 1u) != 0u) runtime.Free(iterator);
    return iterator;
}

std::uint32_t CollectionIteratorLifecycle::DestroyFirst(
    const std::uint32_t iterator, const std::uint32_t flag,
    ICollectionIteratorLifecycleRuntime& runtime) {
    InitFirstBase(iterator, runtime);
    if ((flag & 1u) != 0u) runtime.Free(iterator);
    return iterator;
}

std::uint32_t CollectionIteratorLifecycle::CleanupFirst(
    const std::uint32_t iterator, const std::uint32_t flag,
    ICollectionIteratorLifecycleRuntime& runtime) {
    InitFirstBase(iterator, runtime);
    if ((flag & 1u) != 0u) runtime.Free(iterator);
    return iterator;
}

std::uint32_t CollectionIteratorLifecycle::CleanupSecond(
    const std::uint32_t iterator, const std::uint32_t flag,
    ICollectionIteratorLifecycleRuntime& runtime) {
    InitSecondBase(iterator, runtime);
    if ((flag & 1u) != 0u) runtime.Free(iterator);
    return iterator;
}

}  // namespace pal4
