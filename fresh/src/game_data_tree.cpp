#include "pal4/game_data_tree.h"

#include <new>

namespace pal4 {

std::uint32_t GameDataTree::FreeNodes(
    std::uint32_t root, IGameDataTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (root != nil) {
        (void)FreeNodes(runtime.ReadWord(root + 8u), runtime);
        const auto left = runtime.ReadWord(root);
        (void)runtime.FreeMemory(root);
        root = left;
    }
    return nil;
}

std::uint32_t GameDataTree::Construct(
    const std::uint32_t tree, IGameDataTreeRuntime& runtime) {
    const auto candidate = runtime.AllocateMemory(20u);
    if (candidate == 0u) throw std::bad_alloc();
    runtime.WriteWord(candidate + 16u, 1u);
    runtime.WriteWord(candidate + 4u, 0u);
    if (runtime.SharedNil() == 0u) {
        runtime.SetSharedNil(candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    }
    runtime.SetSharedNilUsers(runtime.SharedNilUsers() + 1u);
    if (candidate != runtime.SharedNil())
        (void)runtime.FreeMemory(candidate);
    const auto head = runtime.AllocateMemory(20u);
    if (head == 0u) throw std::bad_alloc();
    runtime.WriteWord(head + 4u, runtime.SharedNil());
    runtime.WriteWord(head + 16u, 0u);
    runtime.WriteWord(tree + 4u, head);
    runtime.WriteWord(tree + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    return head;
}

std::uint32_t GameDataTree::Destroy(
    const std::uint32_t tree, IGameDataTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto cursor = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        (void)FreeNodes(runtime.ReadWord(head + 4u), runtime);
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.GetFirstElement(tree);
    } else {
        while (cursor != head) {
            const auto node = cursor;
            (void)NextCursor(cursor, runtime);
            DeleteCore(tree, node, runtime);
        }
    }
    (void)runtime.FreeMemory(head);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const auto users = runtime.SharedNilUsers() - 1u;
    runtime.SetSharedNilUsers(users);
    if (users != 0u) return users;
    const auto nil = runtime.SharedNil();
    runtime.SetSharedNil(0u);
    return nil != 0u ? runtime.FreeMemory(nil) : 0u;
}

std::uint32_t GameDataTree::NextCursor(
    std::uint32_t& cursor, IGameDataTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto right = runtime.ReadWord(cursor + 8u);
    if (right != nil) {
        auto next = runtime.ReadWord(right);
        while (next != nil) {
            right = next;
            next = runtime.ReadWord(right);
        }
        cursor = right;
        return next;
    }
    auto ancestor = runtime.ReadWord(cursor + 4u);
    while (cursor == runtime.ReadWord(ancestor + 8u)) {
        cursor = ancestor;
        ancestor = runtime.ReadWord(ancestor + 4u);
    }
    if (runtime.ReadWord(cursor + 8u) != ancestor)
        cursor = ancestor;
    return ancestor;
}

std::uint32_t GameDataTree::PostIncrement(
    std::uint32_t& cursor, const std::uint32_t output,
    IGameDataTreeRuntime& runtime) {
    const auto previous = cursor;
    (void)NextCursor(cursor, runtime);
    runtime.WriteWord(output, previous);
    return output;
}

std::uint32_t GameDataTree::PreviousCursor(
    std::uint32_t& cursor, IGameDataTreeRuntime& runtime) {
    const auto node = cursor;
    if (runtime.ReadWord(node + 16u) == 0u &&
        runtime.ReadWord(runtime.ReadWord(node + 4u) + 4u) == node) {
        cursor = runtime.ReadWord(node + 8u);
        return cursor;
    }
    const auto nil = runtime.SharedNil();
    auto left = runtime.ReadWord(node);
    if (left != nil) {
        auto right = runtime.ReadWord(left + 8u);
        while (right != nil) {
            left = right;
            right = runtime.ReadWord(left + 8u);
        }
        cursor = left;
        return right;
    }
    auto parent = runtime.ReadWord(node + 4u);
    while (cursor == runtime.ReadWord(parent)) {
        cursor = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    cursor = parent;
    return parent;
}

std::uint32_t GameDataTree::DeleteNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t node, IGameDataTreeRuntime& runtime) {
    auto next = node;
    (void)NextCursor(next, runtime);
    DeleteCore(tree, node, runtime);
    runtime.WriteWord(output, next);
    return output;
}

void GameDataTree::DeleteCore(
    const std::uint32_t tree, const std::uint32_t node,
    IGameDataTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    const auto head = runtime.ReadWord(tree + 4u);
    auto removed = node;
    std::uint32_t child;
    if (runtime.ReadWord(node) == nil) {
        child = runtime.ReadWord(node + 8u);
    } else if (runtime.ReadWord(node + 8u) == nil) {
        child = runtime.ReadWord(node);
    } else {
        removed = runtime.ReadWord(node + 8u);
        while (runtime.ReadWord(removed) != nil)
            removed = runtime.ReadWord(removed);
        child = runtime.ReadWord(removed + 8u);
    }

    if (removed != node) {
        const auto left = runtime.ReadWord(node);
        runtime.WriteWord(left + 4u, removed);
        runtime.WriteWord(removed, left);
        if (removed == runtime.ReadWord(node + 8u)) {
            runtime.WriteWord(child + 4u, removed);
        } else {
            const auto old_parent = runtime.ReadWord(removed + 4u);
            runtime.WriteWord(child + 4u, old_parent);
            runtime.WriteWord(old_parent, child);
            const auto right = runtime.ReadWord(node + 8u);
            runtime.WriteWord(removed + 8u, right);
            runtime.WriteWord(right + 4u, removed);
        }
        const auto parent = runtime.ReadWord(node + 4u);
        if (runtime.ReadWord(head + 4u) == node)
            runtime.WriteWord(head + 4u, removed);
        else if (runtime.ReadWord(parent) == node)
            runtime.WriteWord(parent, removed);
        else
            runtime.WriteWord(parent + 8u, removed);
        runtime.WriteWord(removed + 4u, parent);
        const auto colour = runtime.ReadWord(removed + 16u);
        runtime.WriteWord(removed + 16u,
                          runtime.ReadWord(node + 16u));
        runtime.WriteWord(node + 16u, colour);
        removed = node;
    } else {
        const auto parent = runtime.ReadWord(node + 4u);
        runtime.WriteWord(child + 4u, parent);
        if (runtime.ReadWord(head + 4u) == node)
            runtime.WriteWord(head + 4u, child);
        else if (runtime.ReadWord(parent) == node)
            runtime.WriteWord(parent, child);
        else
            runtime.WriteWord(parent + 8u, child);
        if (runtime.ReadWord(head) == node) {
            if (runtime.ReadWord(node + 8u) == nil) {
                runtime.WriteWord(head, parent);
            } else {
                auto first = child;
                while (runtime.ReadWord(first) != nil)
                    first = runtime.ReadWord(first);
                runtime.WriteWord(head, first);
            }
        }
        if (runtime.ReadWord(head + 8u) == node) {
            if (runtime.ReadWord(node) == nil) {
                runtime.WriteWord(head + 8u, parent);
            } else {
                auto last = child;
                while (runtime.ReadWord(last + 8u) != nil)
                    last = runtime.ReadWord(last + 8u);
                runtime.WriteWord(head + 8u, last);
            }
        }
    }

    if (runtime.ReadWord(removed + 16u) == 1u) {
        while (child != runtime.ReadWord(head + 4u) &&
               runtime.ReadWord(child + 16u) == 1u) {
            const auto parent = runtime.ReadWord(child + 4u);
            if (child == runtime.ReadWord(parent)) {
                auto sibling = runtime.ReadWord(parent + 8u);
                if (runtime.ReadWord(sibling + 16u) == 0u) {
                    runtime.WriteWord(sibling + 16u, 1u);
                    runtime.WriteWord(parent + 16u, 0u);
                    RotateLeft(tree, parent, runtime);
                    sibling = runtime.ReadWord(
                        runtime.ReadWord(child + 4u) + 8u);
                }
                if (runtime.ReadWord(runtime.ReadWord(sibling) + 16u) == 1u &&
                    runtime.ReadWord(runtime.ReadWord(sibling + 8u) + 16u) == 1u) {
                    runtime.WriteWord(sibling + 16u, 0u);
                    child = runtime.ReadWord(child + 4u);
                } else {
                    if (runtime.ReadWord(runtime.ReadWord(sibling + 8u) + 16u) == 1u) {
                        runtime.WriteWord(runtime.ReadWord(sibling) + 16u, 1u);
                        runtime.WriteWord(sibling + 16u, 0u);
                        RotateRight(tree, sibling, runtime);
                        sibling = runtime.ReadWord(
                            runtime.ReadWord(child + 4u) + 8u);
                    }
                    const auto current_parent = runtime.ReadWord(child + 4u);
                    runtime.WriteWord(sibling + 16u,
                                      runtime.ReadWord(current_parent + 16u));
                    runtime.WriteWord(current_parent + 16u, 1u);
                    runtime.WriteWord(runtime.ReadWord(sibling + 8u) + 16u, 1u);
                    RotateLeft(tree, current_parent, runtime);
                    break;
                }
            } else {
                auto sibling = runtime.ReadWord(parent);
                if (runtime.ReadWord(sibling + 16u) == 0u) {
                    runtime.WriteWord(sibling + 16u, 1u);
                    runtime.WriteWord(parent + 16u, 0u);
                    RotateRight(tree, parent, runtime);
                    sibling = runtime.ReadWord(
                        runtime.ReadWord(child + 4u));
                }
                if (runtime.ReadWord(runtime.ReadWord(sibling + 8u) + 16u) == 1u &&
                    runtime.ReadWord(runtime.ReadWord(sibling) + 16u) == 1u) {
                    runtime.WriteWord(sibling + 16u, 0u);
                    child = runtime.ReadWord(child + 4u);
                } else {
                    if (runtime.ReadWord(runtime.ReadWord(sibling) + 16u) == 1u) {
                        runtime.WriteWord(runtime.ReadWord(sibling + 8u) + 16u, 1u);
                        runtime.WriteWord(sibling + 16u, 0u);
                        RotateLeft(tree, sibling, runtime);
                        sibling = runtime.ReadWord(
                            runtime.ReadWord(child + 4u));
                    }
                    const auto current_parent = runtime.ReadWord(child + 4u);
                    runtime.WriteWord(sibling + 16u,
                                      runtime.ReadWord(current_parent + 16u));
                    runtime.WriteWord(current_parent + 16u, 1u);
                    runtime.WriteWord(runtime.ReadWord(sibling) + 16u, 1u);
                    RotateRight(tree, current_parent, runtime);
                    break;
                }
            }
        }
        runtime.WriteWord(child + 16u, 1u);
    }
    (void)runtime.FreeMemory(removed);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) - 1u);
}

std::uint32_t GameDataTree::RotateLeft(
    const std::uint32_t tree, const std::uint32_t node,
    IGameDataTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    const auto right = runtime.ReadWord(node + 8u);
    const auto middle = runtime.ReadWord(right);
    runtime.WriteWord(node + 8u, middle);
    if (middle != nil) runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(right + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u)) {
        runtime.WriteWord(head + 4u, right);
    } else if (node == runtime.ReadWord(parent)) {
        runtime.WriteWord(parent, right);
    } else {
        runtime.WriteWord(parent + 8u, right);
    }
    runtime.WriteWord(right, node);
    runtime.WriteWord(node + 4u, right);
    return right;
}

std::uint32_t GameDataTree::RotateRight(
    const std::uint32_t tree, const std::uint32_t node,
    IGameDataTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    const auto left = runtime.ReadWord(node);
    const auto middle = runtime.ReadWord(left + 8u);
    runtime.WriteWord(node, middle);
    if (middle != nil) runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(left + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u)) {
        runtime.WriteWord(head + 4u, left);
    } else if (node == runtime.ReadWord(parent + 8u)) {
        runtime.WriteWord(parent + 8u, left);
    } else {
        runtime.WriteWord(parent, left);
    }
    runtime.WriteWord(left + 8u, node);
    runtime.WriteWord(node + 4u, left);
    return left;
}

std::uint32_t GameDataTree::Previous(
    const std::uint32_t node, IGameDataTreeRuntime& runtime) {
    auto cursor = node;
    (void)PreviousCursor(cursor, runtime);
    return cursor;
}

std::uint32_t GameDataTree::InsertCore(
    const std::uint32_t tree, const std::uint32_t child,
    const std::uint32_t parent, const std::uint32_t key,
    IGameDataTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    const auto node = runtime.AllocateMemory(20u);
    if (node == 0u) throw std::bad_alloc();
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 16u, 0u);
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    runtime.WriteWord(node + 12u, runtime.ReadWord(key));
    const auto head = runtime.ReadWord(tree + 4u);
    runtime.WriteWord(tree + 12u,
                      runtime.ReadWord(tree + 12u) + 1u);
    if (parent == head || child != nil ||
        static_cast<std::int32_t>(runtime.ReadWord(key)) <
            static_cast<std::int32_t>(runtime.ReadWord(parent + 12u))) {
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
        const auto p = runtime.ReadWord(current + 4u);
        if (runtime.ReadWord(p + 16u) != 0u) break;
        const auto grand = runtime.ReadWord(p + 4u);
        if (p == runtime.ReadWord(grand)) {
            const auto uncle = runtime.ReadWord(grand + 8u);
            if (runtime.ReadWord(uncle + 16u) == 0u) {
                runtime.WriteWord(p + 16u, 1u);
                runtime.WriteWord(uncle + 16u, 1u);
                runtime.WriteWord(grand + 16u, 0u);
                current = grand;
            } else {
                if (current == runtime.ReadWord(p + 8u)) {
                    current = p;
                    RotateLeft(tree, current, runtime);
                }
                const auto next_parent = runtime.ReadWord(current + 4u);
                const auto next_grand = runtime.ReadWord(next_parent + 4u);
                runtime.WriteWord(next_parent + 16u, 1u);
                runtime.WriteWord(next_grand + 16u, 0u);
                RotateRight(tree, next_grand, runtime);
            }
        } else {
            const auto uncle = runtime.ReadWord(grand);
            if (runtime.ReadWord(uncle + 16u) == 0u) {
                runtime.WriteWord(p + 16u, 1u);
                runtime.WriteWord(uncle + 16u, 1u);
                runtime.WriteWord(grand + 16u, 0u);
                current = grand;
            } else {
                if (current == runtime.ReadWord(p)) {
                    current = p;
                    RotateRight(tree, current, runtime);
                }
                const auto next_parent = runtime.ReadWord(current + 4u);
                const auto next_grand = runtime.ReadWord(next_parent + 4u);
                runtime.WriteWord(next_parent + 16u, 1u);
                runtime.WriteWord(next_grand + 16u, 0u);
                RotateLeft(tree, next_grand, runtime);
            }
        }
    }
    runtime.WriteWord(runtime.ReadWord(head + 4u) + 16u, 1u);
    return node;
}

std::uint32_t GameDataTree::InsertNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t key, IGameDataTreeRuntime& runtime) {
    runtime.WriteWord(output,
                      InsertCore(tree, child, parent, key, runtime));
    return output;
}

std::uint32_t GameDataTree::InsertOrGet(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t key, IGameDataTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    const auto head = runtime.ReadWord(tree + 4u);
    auto parent = head;
    auto child = runtime.ReadWord(head + 4u);
    bool less = true;
    const auto key_value = static_cast<std::int32_t>(runtime.ReadWord(key));
    while (child != nil) {
        parent = child;
        less = key_value <
            static_cast<std::int32_t>(runtime.ReadWord(child + 12u));
        child = runtime.ReadWord(child + (less ? 0u : 8u));
    }

    auto selected = parent;
    bool inserted = false;
    if (runtime.ReadByte(tree + 8u) != 0u) {
        selected = InsertCore(tree, child, parent, key, runtime);
        inserted = true;
    } else {
        if (less) {
            if (parent == runtime.ReadWord(head)) {
                selected = InsertCore(tree, child, parent, key, runtime);
                inserted = true;
            } else {
                selected = Previous(parent, runtime);
            }
        }
        if (!inserted &&
            static_cast<std::int32_t>(runtime.ReadWord(selected + 12u)) <
                key_value) {
            selected = InsertCore(tree, child, parent, key, runtime);
            inserted = true;
        }
    }
    runtime.WriteWord(output, selected);
    runtime.WriteByte(output + 4u, inserted ? 1u : 0u);
    return output;
}

}  // namespace pal4
