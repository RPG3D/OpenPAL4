#include "pal4/collection_iterator_factory.h"

#include <new>

namespace pal4 {

std::uint32_t CollectionIteratorFactoryRuntimeStub::Allocate(
    std::uint32_t) { return 0u; }
std::uint32_t CollectionIteratorFactoryRuntimeStub::ReadWord(
    std::uint32_t) { return 0u; }
void CollectionIteratorFactoryRuntimeStub::WriteWord(
    std::uint32_t, std::uint32_t) {}

std::uint32_t CollectionIteratorFactory::CreateBasic(
    CollectionIteratorFactoryState& owner, const std::uint32_t argument,
    const std::uint32_t vtable,
    ICollectionIteratorFactoryRuntime& runtime) {
    const auto result = runtime.Allocate(12u);
    if (result != 0u) {
        runtime.WriteWord(result + 4u, 0u);
        runtime.WriteWord(result + 8u, 0u);
        runtime.WriteWord(result, vtable);
        runtime.WriteWord(result + 4u, argument);
        runtime.WriteWord(result + 8u, owner.source);
    } else {
        // The original writes these two words through a null result pointer.
        runtime.WriteWord(4u, argument);
        runtime.WriteWord(8u, owner.source);
    }
    owner.created = result;
    return result;
}

std::uint32_t CollectionIteratorFactory::Create1(
    CollectionIteratorFactoryState& owner, const std::uint32_t argument,
    ICollectionIteratorFactoryRuntime& runtime) {
    return CreateBasic(owner, argument, 0x849748u, runtime);
}
std::uint32_t CollectionIteratorFactory::Create2(
    CollectionIteratorFactoryState& owner, const std::uint32_t argument,
    ICollectionIteratorFactoryRuntime& runtime) {
    return CreateBasic(owner, argument, 0x849760u, runtime);
}
std::uint32_t CollectionIteratorFactory::Create3(
    CollectionIteratorFactoryState& owner, const std::uint32_t argument,
    ICollectionIteratorFactoryRuntime& runtime) {
    return CreateBasic(owner, argument, 0x849778u, runtime);
}
std::uint32_t CollectionIteratorFactory::Create4(
    CollectionIteratorFactoryState& owner, const std::uint32_t argument,
    ICollectionIteratorFactoryRuntime& runtime) {
    return CreateBasic(owner, argument, 0x849790u, runtime);
}
std::uint32_t CollectionIteratorFactory::Create5(
    CollectionIteratorFactoryState& owner, const std::uint32_t argument,
    ICollectionIteratorFactoryRuntime& runtime) {
    return CreateBasic(owner, argument, 0x8497a8u, runtime);
}

std::uint32_t CollectionIteratorFactory::Create6(
    CollectionIteratorFactoryState& owner, const std::uint32_t source_range,
    ICollectionIteratorFactoryRuntime& runtime) {
    const auto result = runtime.Allocate(20u);
    if (result == 0u) throw std::bad_alloc{};
    runtime.WriteWord(result + 4u, 0u);
    runtime.WriteWord(result + 8u, 0u);
    runtime.WriteWord(result + 12u, 0u);
    runtime.WriteWord(result + 16u, 0u);
    runtime.WriteWord(result, 0x8497c0u);
    runtime.WriteWord(result + 12u, source_range);
    runtime.WriteWord(result + 16u, owner.source);
    runtime.WriteWord(result + 8u, runtime.ReadWord(source_range + 8u));
    runtime.WriteWord(result + 4u, runtime.ReadWord(source_range + 4u));
    owner.created = result;
    return result;
}

}  // namespace pal4
