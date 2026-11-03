#include "pal4/actor_dynamic_effect_tree.h"

#include <new>

namespace pal4 {

namespace {

void RotateLeft(std::uint32_t head, std::uint32_t node, std::uint32_t nil,
                IActorDynamicEffectTreeRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node + 8u);
    const auto moved = runtime.ReadWord(pivot);
    runtime.WriteWord(node + 8u, moved);
    if (moved != nil) runtime.WriteWord(moved + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent))
        runtime.WriteWord(parent, pivot);
    else
        runtime.WriteWord(parent + 8u, pivot);
    runtime.WriteWord(pivot, node);
    runtime.WriteWord(node + 4u, pivot);
}

void RotateRight(std::uint32_t head, std::uint32_t node, std::uint32_t nil,
                 IActorDynamicEffectTreeRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node);
    const auto moved = runtime.ReadWord(pivot + 8u);
    runtime.WriteWord(node, moved);
    if (moved != nil) runtime.WriteWord(moved + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent))
        runtime.WriteWord(parent, pivot);
    else
        runtime.WriteWord(parent + 8u, pivot);
    runtime.WriteWord(pivot + 8u, node);
    runtime.WriteWord(node + 4u, pivot);
}

}  // namespace

void ActorDynamicEffectTree::Destroy(
    std::uint32_t tree, IActorDynamicEffectTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto cursor = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        ClearSubtree(runtime.ReadWord(head + 4u), runtime);
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
    } else {
        while (cursor != head) {
            const auto node = cursor;
            cursor = Erase(tree, 0u, node, runtime);
        }
    }

    runtime.Free(head);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const auto references = runtime.SharedReferences() - 1u;
    runtime.SetSharedReferences(references);
    if (references == 0u) {
        const auto nil = runtime.SharedNil();
        runtime.SetSharedNil(0u);
        if (nil != 0u) runtime.Free(nil);
    }
}

std::uint32_t ActorDynamicEffectTree::Erase(
    std::uint32_t tree, std::uint32_t result, std::uint32_t node,
    IActorDynamicEffectTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil();
    const auto successor = Successor(node, runtime);
    auto removed = node;
    std::uint32_t replacement;
    const auto left = runtime.ReadWord(node);
    const auto right = runtime.ReadWord(node + 8u);
    if (left == nil) {
        replacement = right;
    } else if (right == nil) {
        replacement = left;
    } else {
        removed = right;
        while (runtime.ReadWord(removed) != nil)
            removed = runtime.ReadWord(removed);
        replacement = runtime.ReadWord(removed + 8u);
    }

    std::uint32_t replacement_parent;
    if (removed != node) {
        runtime.WriteWord(left + 4u, removed);
        runtime.WriteWord(removed, left);
        if (removed == right) {
            replacement_parent = removed;
        } else {
            replacement_parent = runtime.ReadWord(removed + 4u);
            runtime.WriteWord(replacement + 4u, replacement_parent);
            runtime.WriteWord(replacement_parent, replacement);
            runtime.WriteWord(removed + 8u, right);
            runtime.WriteWord(right + 4u, removed);
        }
        const auto parent = runtime.ReadWord(node + 4u);
        runtime.WriteWord(removed + 4u, parent);
        if (node == runtime.ReadWord(head + 4u))
            runtime.WriteWord(head + 4u, removed);
        else if (node == runtime.ReadWord(parent))
            runtime.WriteWord(parent, removed);
        else
            runtime.WriteWord(parent + 8u, removed);
        const auto color = runtime.ReadWord(node + 24u);
        runtime.WriteWord(node + 24u, runtime.ReadWord(removed + 24u));
        runtime.WriteWord(removed + 24u, color);
        removed = node;
    } else {
        replacement_parent = runtime.ReadWord(node + 4u);
        runtime.WriteWord(replacement + 4u, replacement_parent);
        if (node == runtime.ReadWord(head + 4u))
            runtime.WriteWord(head + 4u, replacement);
        else if (node == runtime.ReadWord(replacement_parent))
            runtime.WriteWord(replacement_parent, replacement);
        else
            runtime.WriteWord(replacement_parent + 8u, replacement);

        if (node == runtime.ReadWord(head)) {
            if (right == nil) {
                runtime.WriteWord(head, replacement_parent);
            } else {
                auto minimum = replacement;
                while (runtime.ReadWord(minimum) != nil)
                    minimum = runtime.ReadWord(minimum);
                runtime.WriteWord(head, minimum);
            }
        }
        if (node == runtime.ReadWord(head + 8u)) {
            if (left == nil) {
                runtime.WriteWord(head + 8u, replacement_parent);
            } else {
                auto maximum = replacement;
                while (runtime.ReadWord(maximum + 8u) != nil)
                    maximum = runtime.ReadWord(maximum + 8u);
                runtime.WriteWord(head + 8u, maximum);
            }
        }
    }

    if (runtime.ReadWord(removed + 24u) == 1u) {
        auto current = replacement;
        while (current != runtime.ReadWord(head + 4u) &&
               runtime.ReadWord(current + 24u) == 1u) {
            const auto parent = replacement_parent;
            if (current == runtime.ReadWord(parent)) {
                auto sibling = runtime.ReadWord(parent + 8u);
                if (runtime.ReadWord(sibling + 24u) == 0u) {
                    runtime.WriteWord(sibling + 24u, 1u);
                    runtime.WriteWord(parent + 24u, 0u);
                    RotateLeft(head, parent, nil, runtime);
                    sibling = runtime.ReadWord(parent + 8u);
                }
                if (runtime.ReadWord(runtime.ReadWord(sibling) + 24u) == 1u &&
                    runtime.ReadWord(runtime.ReadWord(sibling + 8u) + 24u) == 1u) {
                    runtime.WriteWord(sibling + 24u, 0u);
                    current = parent;
                    replacement_parent = runtime.ReadWord(current + 4u);
                } else {
                    if (runtime.ReadWord(runtime.ReadWord(sibling + 8u) + 24u) == 1u) {
                        runtime.WriteWord(runtime.ReadWord(sibling) + 24u, 1u);
                        runtime.WriteWord(sibling + 24u, 0u);
                        RotateRight(head, sibling, nil, runtime);
                        sibling = runtime.ReadWord(parent + 8u);
                    }
                    runtime.WriteWord(sibling + 24u,
                                      runtime.ReadWord(parent + 24u));
                    runtime.WriteWord(parent + 24u, 1u);
                    runtime.WriteWord(runtime.ReadWord(sibling + 8u) + 24u, 1u);
                    RotateLeft(head, parent, nil, runtime);
                    break;
                }
            } else {
                auto sibling = runtime.ReadWord(parent);
                if (runtime.ReadWord(sibling + 24u) == 0u) {
                    runtime.WriteWord(sibling + 24u, 1u);
                    runtime.WriteWord(parent + 24u, 0u);
                    RotateRight(head, parent, nil, runtime);
                    sibling = runtime.ReadWord(parent);
                }
                if (runtime.ReadWord(runtime.ReadWord(sibling + 8u) + 24u) == 1u &&
                    runtime.ReadWord(runtime.ReadWord(sibling) + 24u) == 1u) {
                    runtime.WriteWord(sibling + 24u, 0u);
                    current = parent;
                    replacement_parent = runtime.ReadWord(current + 4u);
                } else {
                    if (runtime.ReadWord(runtime.ReadWord(sibling) + 24u) == 1u) {
                        runtime.WriteWord(runtime.ReadWord(sibling + 8u) + 24u, 1u);
                        runtime.WriteWord(sibling + 24u, 0u);
                        RotateLeft(head, sibling, nil, runtime);
                        sibling = runtime.ReadWord(parent);
                    }
                    runtime.WriteWord(sibling + 24u,
                                      runtime.ReadWord(parent + 24u));
                    runtime.WriteWord(parent + 24u, 1u);
                    runtime.WriteWord(runtime.ReadWord(sibling) + 24u, 1u);
                    RotateRight(head, parent, nil, runtime);
                    break;
                }
            }
        }
        runtime.WriteWord(current + 24u, 1u);
    }
    runtime.ResetHandle(removed + 16u);
    runtime.Free(removed);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) - 1u);
    if (result != 0u) runtime.WriteWord(result, successor);
    return successor;
}

void ActorDynamicEffectTree::EraseRange(
    std::uint32_t tree, std::uint32_t result, std::uint32_t first,
    std::uint32_t last, IActorDynamicEffectTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    if (runtime.ReadWord(tree + 12u) != 0u &&
        first == runtime.ReadWord(head) && last == head) {
        ClearSubtree(runtime.ReadWord(head + 4u), runtime);
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.WriteWord(result, head);
        return;
    }
    auto cursor = first;
    while (cursor != last) {
        cursor = Erase(tree, result, cursor, runtime);
    }
    runtime.WriteWord(result, cursor);
}

std::uint32_t ActorDynamicEffectTree::Successor(
    std::uint32_t node, IActorDynamicEffectTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto right = runtime.ReadWord(node + 8u);
    if (right != nil) {
        while (runtime.ReadWord(right) != nil)
            right = runtime.ReadWord(right);
        return right;
    }
    auto parent = runtime.ReadWord(node + 4u);
    while (node == runtime.ReadWord(parent + 8u)) {
        node = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    return runtime.ReadWord(node + 8u) == parent ? node : parent;
}

void ActorDynamicEffectTree::ClearSubtree(
    std::uint32_t root, IActorDynamicEffectTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (root != nil) {
        ClearSubtree(runtime.ReadWord(root + 8u), runtime);
        const auto left = runtime.ReadWord(root);
        runtime.ResetHandle(root + 16u);
        runtime.Free(root);
        root = left;
    }
}

std::uint32_t ActorDynamicEffectTree::InsertAt(
    std::uint32_t tree, std::uint32_t cursor, std::uint32_t parent,
    std::uint32_t record, IActorDynamicEffectTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil();
    const auto node = runtime.Allocate(28u);
    if (node == 0u) throw std::bad_alloc();
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 24u, 0u);
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    (void)ActorEffectRecord::Copy(node + 12u, record, runtime);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) + 1u);

    const auto key = static_cast<std::int32_t>(runtime.ReadWord(record));
    const bool insert_left = parent == head || cursor != nil ||
        key < static_cast<std::int32_t>(runtime.ReadWord(parent + 12u));
    if (insert_left) {
        runtime.WriteWord(parent, node);
        if (parent == head) {
            runtime.WriteWord(head + 4u, node);
            runtime.WriteWord(head + 8u, node);
        } else if (parent == runtime.ReadWord(head)) {
            runtime.WriteWord(head, node);
        }
    } else {
        runtime.WriteWord(parent + 8u, node);
        if (parent == runtime.ReadWord(head + 8u))
            runtime.WriteWord(head + 8u, node);
    }

    auto current = node;
    while (current != runtime.ReadWord(head + 4u)) {
        const auto current_parent = runtime.ReadWord(current + 4u);
        if (runtime.ReadWord(current_parent + 24u) != 0u) break;
        const auto grandparent = runtime.ReadWord(current_parent + 4u);
        if (current_parent == runtime.ReadWord(grandparent)) {
            const auto uncle = runtime.ReadWord(grandparent + 8u);
            if (runtime.ReadWord(uncle + 24u) == 0u) {
                runtime.WriteWord(current_parent + 24u, 1u);
                runtime.WriteWord(uncle + 24u, 1u);
                runtime.WriteWord(grandparent + 24u, 0u);
                current = grandparent;
            } else {
                if (current == runtime.ReadWord(current_parent + 8u)) {
                    current = current_parent;
                    RotateLeft(head, current, nil, runtime);
                }
                const auto next_parent = runtime.ReadWord(current + 4u);
                const auto next_grandparent = runtime.ReadWord(next_parent + 4u);
                runtime.WriteWord(next_parent + 24u, 1u);
                runtime.WriteWord(next_grandparent + 24u, 0u);
                RotateRight(head, next_grandparent, nil, runtime);
            }
        } else {
            const auto uncle = runtime.ReadWord(grandparent);
            if (runtime.ReadWord(uncle + 24u) == 0u) {
                runtime.WriteWord(current_parent + 24u, 1u);
                runtime.WriteWord(uncle + 24u, 1u);
                runtime.WriteWord(grandparent + 24u, 0u);
                current = grandparent;
            } else {
                if (current == runtime.ReadWord(current_parent)) {
                    current = current_parent;
                    RotateRight(head, current, nil, runtime);
                }
                const auto next_parent = runtime.ReadWord(current + 4u);
                const auto next_grandparent = runtime.ReadWord(next_parent + 4u);
                runtime.WriteWord(next_parent + 24u, 1u);
                runtime.WriteWord(next_grandparent + 24u, 0u);
                RotateLeft(head, next_grandparent, nil, runtime);
            }
        }
    }
    runtime.WriteWord(runtime.ReadWord(head + 4u) + 24u, 1u);
    return node;
}

std::uint32_t ActorDynamicEffectTree::Leftmost(
    std::uint32_t node, IActorDynamicEffectTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (runtime.ReadWord(node) != nil)
        node = runtime.ReadWord(node);
    return node;
}

std::uint32_t ActorDynamicEffectTree::LowerBound(
    std::uint32_t tree, std::int32_t key,
    IActorDynamicEffectTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil();
    auto candidate = head;
    auto cursor = runtime.ReadWord(head + 4u);
    while (cursor != nil) {
        if (static_cast<std::int32_t>(runtime.ReadWord(cursor + 12u)) >= key) {
            candidate = cursor;
            cursor = runtime.ReadWord(cursor);
        } else {
            cursor = runtime.ReadWord(cursor + 8u);
        }
    }
    return candidate;
}

std::uint32_t ActorDynamicEffectTree::Predecessor(
    std::uint32_t node, IActorDynamicEffectTreeRuntime& runtime) {
    const auto parent = runtime.ReadWord(node + 4u);
    if (runtime.ReadWord(node + 24u) == 0u &&
        runtime.ReadWord(parent + 4u) == node)
        return runtime.ReadWord(node + 8u);

    const auto nil = runtime.SharedNil();
    auto left = runtime.ReadWord(node);
    if (left != nil) {
        while (runtime.ReadWord(left + 8u) != nil)
            left = runtime.ReadWord(left + 8u);
        return left;
    }
    auto ancestor = parent;
    while (node == runtime.ReadWord(ancestor)) {
        node = ancestor;
        ancestor = runtime.ReadWord(ancestor + 4u);
    }
    return ancestor;
}

}  // namespace pal4
