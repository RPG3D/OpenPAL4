#include "pal4/combat_reward_tree_insertion.h"

#include <new>

namespace pal4 {

std::uint32_t CombatRewardTreeInsertion::RotateLeft(
    std::uint32_t tree, std::uint32_t node,
    ICombatRewardTreeInsertionRuntime& runtime) {
    const auto nil = runtime.Nil();
    const auto right = runtime.ReadWord(node + 8u);
    const auto middle = runtime.ReadWord(right);
    runtime.WriteWord(node + 8u, middle);
    if (middle != nil) runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(right + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, right);
    else if (node == runtime.ReadWord(parent))
        runtime.WriteWord(parent, right);
    else
        runtime.WriteWord(parent + 8u, right);
    runtime.WriteWord(right, node);
    runtime.WriteWord(node + 4u, right);
    return right;
}

std::uint32_t CombatRewardTreeInsertion::RotateRight(
    std::uint32_t tree, std::uint32_t node,
    ICombatRewardTreeInsertionRuntime& runtime) {
    const auto nil = runtime.Nil();
    const auto left = runtime.ReadWord(node);
    const auto middle = runtime.ReadWord(left + 8u);
    runtime.WriteWord(node, middle);
    if (middle != nil) runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(left + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, left);
    else if (node == runtime.ReadWord(parent + 8u))
        runtime.WriteWord(parent + 8u, left);
    else
        runtime.WriteWord(parent, left);
    runtime.WriteWord(left + 8u, node);
    runtime.WriteWord(node + 4u, left);
    return left;
}

std::uint32_t CombatRewardTreeInsertion::InsertAndRebalance(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t child, std::uint32_t parent,
    std::uint32_t key, ICombatRewardTreeInsertionRuntime& runtime) {
    const auto nil = runtime.Nil();
    const auto head = runtime.ReadWord(tree + 4u);
    const auto inserted = runtime.Allocate(24u);
    if (inserted == 0u) throw std::bad_alloc();
    runtime.WriteWord(inserted + 4u, parent);
    runtime.WriteWord(inserted + 20u, 0u);
    runtime.WriteWord(inserted, nil);
    runtime.WriteWord(inserted + 8u, nil);
    runtime.WriteWord(inserted + 12u, runtime.ReadWord(key));
    runtime.WriteWord(inserted + 16u, runtime.ReadWord(key + 4u));
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) + 1u);

    const auto value = static_cast<std::int32_t>(runtime.ReadWord(key));
    const bool left = parent == head || child != nil ||
        value < static_cast<std::int32_t>(runtime.ReadWord(parent + 12u));
    if (left) {
        runtime.WriteWord(parent, inserted);
        if (parent == head) {
            runtime.WriteWord(head + 4u, inserted);
            runtime.WriteWord(head + 8u, inserted);
        } else if (parent == runtime.ReadWord(head)) {
            runtime.WriteWord(head, inserted);
        }
    } else {
        runtime.WriteWord(parent + 8u, inserted);
        if (parent == runtime.ReadWord(head + 8u))
            runtime.WriteWord(head + 8u, inserted);
    }

    auto current = inserted;
    while (current != runtime.ReadWord(head + 4u)) {
        const auto p = runtime.ReadWord(current + 4u);
        if (runtime.ReadWord(p + 20u) != 0u) break;
        const auto grand = runtime.ReadWord(p + 4u);
        if (p == runtime.ReadWord(grand)) {
            const auto uncle = runtime.ReadWord(grand + 8u);
            if (runtime.ReadWord(uncle + 20u) == 0u) {
                runtime.WriteWord(p + 20u, 1u);
                runtime.WriteWord(uncle + 20u, 1u);
                runtime.WriteWord(grand + 20u, 0u);
                current = grand;
            } else {
                if (current == runtime.ReadWord(p + 8u)) {
                    current = p;
                    (void)RotateLeft(tree, current, runtime);
                }
                const auto next_parent = runtime.ReadWord(current + 4u);
                const auto next_grand = runtime.ReadWord(next_parent + 4u);
                runtime.WriteWord(next_parent + 20u, 1u);
                runtime.WriteWord(next_grand + 20u, 0u);
                (void)RotateRight(tree, next_grand, runtime);
            }
        } else {
            const auto uncle = runtime.ReadWord(grand);
            if (runtime.ReadWord(uncle + 20u) == 0u) {
                runtime.WriteWord(p + 20u, 1u);
                runtime.WriteWord(uncle + 20u, 1u);
                runtime.WriteWord(grand + 20u, 0u);
                current = grand;
            } else {
                if (current == runtime.ReadWord(p)) {
                    current = p;
                    (void)RotateRight(tree, current, runtime);
                }
                const auto next_parent = runtime.ReadWord(current + 4u);
                const auto next_grand = runtime.ReadWord(next_parent + 4u);
                runtime.WriteWord(next_parent + 20u, 1u);
                runtime.WriteWord(next_grand + 20u, 0u);
                (void)RotateLeft(tree, next_grand, runtime);
            }
        }
    }
    runtime.WriteWord(runtime.ReadWord(head + 4u) + 20u, 1u);
    runtime.WriteWord(result, inserted);
    return result;
}

}  // namespace pal4
