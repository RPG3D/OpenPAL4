#include "pal4/sound_pool_tree.h"

namespace pal4 {

std::uint32_t SoundPoolTree::RotateLeft(
    const std::uint32_t pool, const std::uint32_t node,
    ISoundPoolTreeRuntime& runtime) {
    const auto right = runtime.ReadWord(node, 8u);
    const auto moved = runtime.ReadWord(right, 0u);
    runtime.WriteWord(node, 8u, moved);
    if (moved != runtime.Sentinel())
        runtime.WriteWord(moved, 4u, node);
    const auto parent = runtime.ReadWord(node, 4u);
    runtime.WriteWord(right, 4u, parent);
    const auto head = runtime.ReadWord(pool, 4u);
    if (node == runtime.ReadWord(head, 4u))
        runtime.WriteWord(head, 4u, right);
    else if (node == runtime.ReadWord(parent, 0u))
        runtime.WriteWord(parent, 0u, right);
    else
        runtime.WriteWord(parent, 8u, right);
    runtime.WriteWord(right, 0u, node);
    runtime.WriteWord(node, 4u, right);
    return right;
}

std::uint32_t SoundPoolTree::RotateRight(
    const std::uint32_t pool, const std::uint32_t node,
    ISoundPoolTreeRuntime& runtime) {
    const auto left = runtime.ReadWord(node, 0u);
    const auto moved = runtime.ReadWord(left, 8u);
    runtime.WriteWord(node, 0u, moved);
    if (moved != runtime.Sentinel())
        runtime.WriteWord(moved, 4u, node);
    const auto parent = runtime.ReadWord(node, 4u);
    runtime.WriteWord(left, 4u, parent);
    const auto head = runtime.ReadWord(pool, 4u);
    if (node == runtime.ReadWord(head, 4u))
        runtime.WriteWord(head, 4u, left);
    else if (node == runtime.ReadWord(parent, 8u))
        runtime.WriteWord(parent, 8u, left);
    else
        runtime.WriteWord(parent, 0u, left);
    runtime.WriteWord(left, 8u, node);
    runtime.WriteWord(node, 4u, left);
    return left;
}

std::uint32_t SoundPoolTree::Previous(
    std::uint32_t node, ISoundPoolTreeRuntime& runtime) {
    const auto parent = runtime.ReadWord(node, 4u);
    if (runtime.ReadWord(node, 24u) == 0u &&
        runtime.ReadWord(parent, 4u) == node)
        return runtime.ReadWord(node, 8u);

    const auto nil = runtime.Sentinel();
    auto left = runtime.ReadWord(node, 0u);
    if (left != nil) {
        while (runtime.ReadWord(left, 8u) != nil)
            left = runtime.ReadWord(left, 8u);
        return left;
    }
    auto ancestor = parent;
    while (node == runtime.ReadWord(ancestor, 0u)) {
        node = ancestor;
        ancestor = runtime.ReadWord(ancestor, 4u);
    }
    return ancestor;
}

}  // namespace pal4
