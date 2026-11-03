#include "pal4/pal_object_manager_traversal.h"

namespace pal4 {
namespace {

std::uint32_t Traverse(
    const std::uint32_t object, const std::uint32_t argument,
    const std::uint32_t slot,
    IPalObjectManagerTraversalRuntime& runtime) {
    PalObjectManagerIterator source{};
    PalObjectManagerIterator iterator{};
    runtime.InitializeIterator(source, runtime.ReadWord(object + 16u));
    runtime.WrapIterator(iterator, source);
    auto result = runtime.IsEnd(iterator);
    while ((result & 0xffu) == 0u) {
        const auto instance = runtime.GetInstance(iterator);
        const auto target = runtime.ReadWord(instance + 16u);
        runtime.Invoke(target, slot, argument);
        runtime.Remove(iterator, 0u);
        result = runtime.IsEnd(iterator);
    }
    return result;
}

}  // namespace

std::uint32_t PalObjectManagerTraversal::Process(
    const std::uint32_t object, const std::uint32_t argument,
    IPalObjectManagerTraversalRuntime& runtime) {
    return Traverse(object, argument, 8u, runtime);
}

std::uint32_t PalObjectManagerTraversal::ProcessWhenPresent(
    const std::uint32_t object, const std::uint32_t argument,
    const std::uint32_t unused,
    IPalObjectManagerTraversalRuntime& runtime) {
    (void)unused;
    const auto empty = runtime.IsPropertyEmpty(object);
    if ((empty & 0xffu) != 0u) return empty;
    return Traverse(object, argument, 12u, runtime);
}

}  // namespace pal4
