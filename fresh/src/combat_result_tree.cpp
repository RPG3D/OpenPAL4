#include "pal4/combat_result_tree.h"

namespace pal4 {
namespace {

std::uint32_t Successor(const std::uint32_t start,
                        ICombatResultTreeRuntime& runtime) {
    const auto nil = runtime.Nil();
    auto right = runtime.ReadWord(start + 8u);
    if (right != nil) {
        while (runtime.ReadWord(right) != nil)
            right = runtime.ReadWord(right);
        return right;
    }
    auto node = start;
    auto parent = runtime.ReadWord(node + 4u);
    while (node == runtime.ReadWord(parent + 8u)) {
        node = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    if (runtime.ReadWord(node + 8u) != parent)
        node = parent;
    return node;
}

void RotateLeft(const std::uint32_t tree, const std::uint32_t node,
                ICombatResultTreeRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node + 8u);
    const auto moved = runtime.ReadWord(pivot);
    runtime.WriteWord(node + 8u, moved);
    if (moved != runtime.Nil())
        runtime.WriteWord(moved + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent + 8u))
        runtime.WriteWord(parent + 8u, pivot);
    else
        runtime.WriteWord(parent, pivot);
    runtime.WriteWord(pivot, node);
    runtime.WriteWord(node + 4u, pivot);
}

void RotateRight(const std::uint32_t tree, const std::uint32_t node,
                 ICombatResultTreeRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node);
    const auto moved = runtime.ReadWord(pivot + 8u);
    runtime.WriteWord(node, moved);
    if (moved != runtime.Nil())
        runtime.WriteWord(moved + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent))
        runtime.WriteWord(parent, pivot);
    else
        runtime.WriteWord(parent + 8u, pivot);
    runtime.WriteWord(pivot + 8u, node);
    runtime.WriteWord(node + 4u, pivot);
}

std::uint32_t EraseNodeImpl(const std::uint32_t tree,
                            const std::uint32_t node,
                            ICombatResultTreeRuntime& runtime) {
    const auto nil = runtime.Nil();
    const auto head = runtime.ReadWord(tree + 4u);
    const auto successor = Successor(node, runtime);
    const auto left = runtime.ReadWord(node);
    const auto right = runtime.ReadWord(node + 8u);
    auto removed = node;
    std::uint32_t replacement;
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
            runtime.WriteWord(replacement + 4u, removed);
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
        const auto color = runtime.ReadWord(node + 20u);
        runtime.WriteWord(node + 20u,
                          runtime.ReadWord(removed + 20u));
        runtime.WriteWord(removed + 20u, color);
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

    if (runtime.ReadWord(removed + 20u) == 1u) {
        auto current = replacement;
        while (current != runtime.ReadWord(head + 4u) &&
               runtime.ReadWord(current + 20u) == 1u) {
            const auto parent = replacement_parent;
            if (current == runtime.ReadWord(parent)) {
                auto sibling = runtime.ReadWord(parent + 8u);
                if (runtime.ReadWord(sibling + 20u) == 0u) {
                    runtime.WriteWord(sibling + 20u, 1u);
                    runtime.WriteWord(parent + 20u, 0u);
                    RotateLeft(tree, parent, runtime);
                    sibling = runtime.ReadWord(parent + 8u);
                }
                if (runtime.ReadWord(runtime.ReadWord(sibling) + 20u) == 1u &&
                    runtime.ReadWord(runtime.ReadWord(sibling + 8u) + 20u) == 1u) {
                    runtime.WriteWord(sibling + 20u, 0u);
                    current = parent;
                    replacement_parent = runtime.ReadWord(current + 4u);
                } else {
                    if (runtime.ReadWord(runtime.ReadWord(sibling + 8u) + 20u) == 1u) {
                        runtime.WriteWord(runtime.ReadWord(sibling) + 20u, 1u);
                        runtime.WriteWord(sibling + 20u, 0u);
                        RotateRight(tree, sibling, runtime);
                        sibling = runtime.ReadWord(parent + 8u);
                    }
                    runtime.WriteWord(sibling + 20u,
                                      runtime.ReadWord(parent + 20u));
                    runtime.WriteWord(parent + 20u, 1u);
                    runtime.WriteWord(runtime.ReadWord(sibling + 8u) + 20u, 1u);
                    RotateLeft(tree, parent, runtime);
                    break;
                }
            } else {
                auto sibling = runtime.ReadWord(parent);
                if (runtime.ReadWord(sibling + 20u) == 0u) {
                    runtime.WriteWord(sibling + 20u, 1u);
                    runtime.WriteWord(parent + 20u, 0u);
                    RotateRight(tree, parent, runtime);
                    sibling = runtime.ReadWord(parent);
                }
                if (runtime.ReadWord(runtime.ReadWord(sibling + 8u) + 20u) == 1u &&
                    runtime.ReadWord(runtime.ReadWord(sibling) + 20u) == 1u) {
                    runtime.WriteWord(sibling + 20u, 0u);
                    current = parent;
                    replacement_parent = runtime.ReadWord(current + 4u);
                } else {
                    if (runtime.ReadWord(runtime.ReadWord(sibling) + 20u) == 1u) {
                        runtime.WriteWord(runtime.ReadWord(sibling + 8u) + 20u, 1u);
                        runtime.WriteWord(sibling + 20u, 0u);
                        RotateLeft(tree, sibling, runtime);
                        sibling = runtime.ReadWord(parent);
                    }
                    runtime.WriteWord(sibling + 20u,
                                      runtime.ReadWord(parent + 20u));
                    runtime.WriteWord(parent + 20u, 1u);
                    runtime.WriteWord(runtime.ReadWord(sibling) + 20u, 1u);
                    RotateRight(tree, parent, runtime);
                    break;
                }
            }
        }
        runtime.WriteWord(current + 20u, 1u);
    }
    runtime.Free(removed);
    runtime.WriteWord(tree + 12u,
                      runtime.ReadWord(tree + 12u) - 1u);
    return successor;
}

void Clear(const std::uint32_t tree, const std::uint32_t head,
           ICombatResultTreeRuntime& runtime) {
    const auto nil = runtime.Nil();
    auto node = runtime.ReadWord(head + 4u);
    while (node != nil) {
        (void)CombatResultTree::DeleteSubtree(runtime.ReadWord(node + 8u), runtime);
        const auto left = runtime.ReadWord(node);
        runtime.Free(node);
        node = left;
    }
    runtime.WriteWord(head + 4u, nil);
    runtime.WriteWord(tree + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
}

}  // namespace

std::uint32_t CombatResultTree::Assign(
    const std::uint32_t tree, const std::uint32_t source,
    ICombatResultTreeRuntime& runtime) {
    if (tree == source)
        return tree;

    const auto head = runtime.ReadWord(tree + 4u);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        Clear(tree, head, runtime);
    } else {
        auto node = runtime.ReadWord(head);
        while (node != head) {
            const auto current = node;
            node = Successor(node, runtime);
            (void)EraseNodeImpl(tree, current, runtime);
        }
    }

    runtime.WriteByte(tree + 1u, runtime.ReadByte(source + 1u));
    const auto source_head = runtime.ReadWord(source + 4u);
    auto source_node = runtime.ReadWord(source_head + 4u);
    const auto nil = runtime.Nil();
    auto first = source_node;
    auto parent = head;
    while (source_node != nil) {
        const auto created = runtime.CreateNode(
            parent, runtime.ReadWord(source_node + 20u));
        if (first == source_node)
            first = created;
        runtime.WriteWord(created + 8u,
                          CopyTree(runtime.ReadWord(source_node + 8u),
                                   created, runtime));
        runtime.CopyData(created + 12u, source_node + 12u);
        runtime.WriteWord(parent, created);
        source_node = runtime.ReadWord(source_node);
        parent = created;
    }
    runtime.WriteWord(parent, nil);
    runtime.WriteWord(head + 4u, first);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(source + 12u));
    if (first == nil) {
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        return tree;
    }
    auto leftmost = first;
    while (runtime.ReadWord(leftmost) != nil)
        leftmost = runtime.ReadWord(leftmost);
    runtime.WriteWord(head, leftmost);
    auto rightmost = first;
    while (runtime.ReadWord(rightmost + 8u) != nil)
        rightmost = runtime.ReadWord(rightmost + 8u);
    runtime.WriteWord(head + 8u, rightmost);
    return tree;
}

std::uint32_t CombatResultTree::EraseRange(
    const std::uint32_t tree, const std::uint32_t result,
    const std::uint32_t first, const std::uint32_t last,
    ICombatResultTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    if (runtime.ReadWord(tree + 12u) != 0u &&
        first == runtime.ReadWord(head) && last == head) {
        Clear(tree, head, runtime);
        runtime.WriteWord(result, runtime.ReadWord(head));
        return result;
    }
    auto node = first;
    while (node != last) {
        const auto removed = node;
        node = Successor(node, runtime);
        (void)EraseNodeImpl(tree, removed, runtime);
    }
    runtime.WriteWord(result, node);
    return result;
}

std::uint32_t CombatResultTree::Next(
    const std::uint32_t iterator,
    ICombatResultTreeRuntime& runtime) {
    const auto node = runtime.ReadWord(iterator);
    const auto nil = runtime.Nil();
    const auto right = runtime.ReadWord(node + 8u);
    if (right != nil) {
        auto leftmost = right;
        while (runtime.ReadWord(leftmost) != nil)
            leftmost = runtime.ReadWord(leftmost);
        runtime.WriteWord(iterator, leftmost);
        return nil;
    }
    auto current = node;
    auto parent = runtime.ReadWord(current + 4u);
    while (current == runtime.ReadWord(parent + 8u)) {
        current = parent;
        runtime.WriteWord(iterator, current);
        parent = runtime.ReadWord(parent + 4u);
    }
    if (runtime.ReadWord(current + 8u) != parent)
        runtime.WriteWord(iterator, parent);
    return parent;
}

std::uint32_t CombatResultTree::CreateListNode(
    const std::uint32_t previous, const std::uint32_t next,
    ICombatResultTreeRuntime& runtime) {
    const auto node = runtime.Allocate(16u);
    runtime.WriteWord(node, previous != 0u ? previous : node);
    runtime.WriteWord(node + 4u, next != 0u ? next : node);
    return node;
}

std::uint32_t CombatResultTree::EraseNode(
    const std::uint32_t tree, const std::uint32_t result,
    const std::uint32_t node, ICombatResultTreeRuntime& runtime) {
    const auto successor = EraseNodeImpl(tree, node, runtime);
    runtime.WriteWord(result, successor);
    return result;
}

std::uint32_t CombatResultTree::DeleteSubtree(
    const std::uint32_t root, ICombatResultTreeRuntime& runtime) {
    const auto nil = runtime.Nil();
    auto node = root;
    while (node != nil) {
        (void)DeleteSubtree(runtime.ReadWord(node + 8u), runtime);
        const auto left = runtime.ReadWord(node);
        runtime.Free(node);
        node = left;
    }
    return nil;
}

std::uint32_t CombatResultTree::FindMin(
    const std::uint32_t slot, ICombatResultTreeRuntime& runtime) {
    auto result = slot;
    auto node = runtime.ReadWord(slot);
    while (node != runtime.Nil()) {
        result = node;
        node = runtime.ReadWord(node);
    }
    return result;
}

std::uint32_t CombatResultTree::Advance(
    const std::uint32_t iterator, const std::uint32_t previous,
    ICombatResultTreeRuntime& runtime) {
    const auto old_node = runtime.ReadWord(iterator);
    auto right = runtime.ReadWord(old_node + 8u);
    if (right != runtime.Nil()) {
        while (runtime.ReadWord(right) != runtime.Nil())
            right = runtime.ReadWord(right);
        runtime.WriteWord(iterator, right);
    } else {
        auto node = old_node;
        auto parent = runtime.ReadWord(node + 4u);
        while (node == runtime.ReadWord(parent + 8u)) {
            node = parent;
            runtime.WriteWord(iterator, node);
            parent = runtime.ReadWord(parent + 4u);
        }
        if (runtime.ReadWord(node + 8u) != parent)
            runtime.WriteWord(iterator, parent);
    }
    runtime.WriteWord(previous, old_node);
    return previous;
}

std::uint32_t CombatResultTree::CopyTree(
    const std::uint32_t root, const std::uint32_t parent,
    ICombatResultTreeRuntime& runtime) {
    const auto nil = runtime.Nil();
    if (root == nil) {
        runtime.WriteWord(parent, nil);
        return nil;
    }

    auto source = root;
    auto parent_node = parent;
    auto first = nil;
    while (source != nil) {
        const auto node = runtime.Allocate(24u);
        runtime.WriteWord(node + 4u, parent_node);
        runtime.WriteWord(node + 20u, runtime.ReadWord(source + 20u));
        if (first == nil)
            first = node;
        runtime.WriteWord(node + 8u,
                          CopyTree(runtime.ReadWord(source + 8u), node, runtime));
        runtime.WriteWord(node + 12u, runtime.ReadWord(source + 12u));
        runtime.WriteWord(node + 16u, runtime.ReadWord(source + 16u));
        runtime.WriteWord(parent_node, node);
        source = runtime.ReadWord(source);
        parent_node = node;
    }
    runtime.WriteWord(parent_node, nil);
    return first;
}

}  // namespace pal4
