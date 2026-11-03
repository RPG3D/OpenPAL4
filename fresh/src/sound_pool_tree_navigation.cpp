#include "pal4/sound_pool_tree.h"

namespace pal4 {

std::uint32_t SoundPoolTree::FindExact(
    const std::uint32_t pool, const std::uint32_t key,
    ISoundPoolTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(pool, 4u);
    const auto candidate = LowerBound(pool, key, runtime);
    if (candidate == head || key < runtime.ReadWord(candidate, 12u))
        return head;
    return candidate;
}

std::uint32_t SoundPoolTree::Next(
    std::uint32_t node, ISoundPoolTreeRuntime& runtime) {
    const auto nil = runtime.Sentinel();
    auto right = runtime.ReadWord(node, 8u);
    if (right != nil) {
        while (runtime.ReadWord(right, 0u) != nil)
            right = runtime.ReadWord(right, 0u);
        return right;
    }
    auto parent = runtime.ReadWord(node, 4u);
    while (node == runtime.ReadWord(parent, 8u)) {
        node = parent;
        parent = runtime.ReadWord(parent, 4u);
    }
    if (runtime.ReadWord(node, 8u) != parent)
        node = parent;
    return node;
}

std::uint32_t SoundPoolTree::LowerBound(
    const std::uint32_t pool, const std::uint32_t key,
    ISoundPoolTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(pool, 4u);
    const auto nil = runtime.Sentinel();
    auto candidate = head;
    auto node = runtime.ReadWord(head, 4u);
    while (node != nil) {
        if (runtime.ReadWord(node, 12u) >= key) {
            candidate = node;
            node = runtime.ReadWord(node, 0u);
        } else {
            node = runtime.ReadWord(node, 8u);
        }
    }
    return candidate;
}

std::uint32_t SoundPoolTree::DestroySubtree(
    std::uint32_t node, ISoundPoolTreeEraseRuntime& runtime) {
    const auto nil = runtime.Sentinel();
    while (node != nil) {
        (void)DestroySubtree(runtime.ReadWord(node, 8u), runtime);
        const auto left = runtime.ReadWord(node, 0u);
        runtime.CleanupEntry(node + 12u);
        runtime.FreeNode(node);
        node = left;
    }
    return nil;
}

std::uint32_t SoundPoolTree::CopyEntry(
    const std::uint32_t destination, const std::uint32_t source,
    ISoundPoolTreeRuntime& runtime) {
    if (destination == 0u)
        return 0u;
    runtime.WriteWord(destination, 0u, runtime.ReadWord(source, 0u));
    runtime.WriteByte(destination, 4u, runtime.ReadByte(source, 4u));
    runtime.WriteByte(source, 4u, 0u);
    const auto file = runtime.ReadWord(source, 8u);
    runtime.WriteWord(destination, 8u, file);
    return file;
}

}  // namespace pal4
