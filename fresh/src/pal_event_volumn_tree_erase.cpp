#include "pal4/pal_event_volumn_manager.h"

#include <bit>
#include <cstdint>

namespace pal4 {
namespace {

class TreeAccess final {
public:
    TreeAccess(std::uint32_t head, IPalEventVolumnTreeEraseRuntime& runtime)
        : head_(head), runtime_(runtime), nil_(runtime.SharedNil()) {}

    [[nodiscard]] std::uint32_t Nil() const { return nil_; }
    [[nodiscard]] std::uint32_t Root() { return Get(head_, 4u); }
    [[nodiscard]] std::uint32_t Get(std::uint32_t node,
                                    std::uint32_t offset = 0u) {
        return runtime_.ReadWord(node + offset);
    }
    void Set(std::uint32_t node, std::uint32_t offset,
             std::uint32_t value) {
        runtime_.WriteWord(node + offset, value);
    }
    [[nodiscard]] std::uint32_t Left(std::uint32_t node) {
        return Get(node);
    }
    [[nodiscard]] std::uint32_t Right(std::uint32_t node) {
        return Get(node, 8u);
    }
    [[nodiscard]] std::uint32_t Parent(std::uint32_t node) {
        return Get(node, 4u);
    }
    [[nodiscard]] std::uint32_t Color(std::uint32_t node) {
        return Get(node, 20u);
    }
    void SetColor(std::uint32_t node, std::uint32_t color) {
        Set(node, 20u, color);
    }
    [[nodiscard]] std::uint32_t Minimum(std::uint32_t node) {
        while (Left(node) != nil_) node = Left(node);
        return node;
    }
    [[nodiscard]] std::uint32_t Maximum(std::uint32_t node) {
        while (Right(node) != nil_) node = Right(node);
        return node;
    }
    void ReplaceLink(std::uint32_t old_node, std::uint32_t replacement) {
        const auto parent = Parent(old_node);
        if (old_node == Root())
            Set(head_, 4u, replacement);
        else if (old_node == Left(parent))
            Set(parent, 0u, replacement);
        else
            Set(parent, 8u, replacement);
    }
    void RotateLeft(std::uint32_t node) {
        const auto pivot = Right(node);
        const auto moved = Left(pivot);
        Set(node, 8u, moved);
        if (moved != nil_) Set(moved, 4u, node);
        ReplaceLink(node, pivot);
        Set(pivot, 4u, Parent(node));
        Set(pivot, 0u, node);
        Set(node, 4u, pivot);
    }
    void RotateRight(std::uint32_t node) {
        const auto pivot = Left(node);
        const auto moved = Right(pivot);
        Set(node, 0u, moved);
        if (moved != nil_) Set(moved, 4u, node);
        ReplaceLink(node, pivot);
        Set(pivot, 4u, Parent(node));
        Set(pivot, 8u, node);
        Set(node, 4u, pivot);
    }
    void RepairAfterErase(std::uint32_t child,
                          std::uint32_t parent) {
        while (child != Root() && Color(child) == 1u) {
            if (child == Left(parent)) {
                auto sibling = Right(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateLeft(parent);
                    sibling = Right(parent);
                }
                if (Color(Left(sibling)) == 1u &&
                    Color(Right(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    child = parent;
                    parent = Parent(child);
                } else {
                    if (Color(Right(sibling)) == 1u) {
                        SetColor(Left(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateRight(sibling);
                        sibling = Right(parent);
                    }
                    SetColor(sibling, Color(parent));
                    SetColor(parent, 1u);
                    SetColor(Right(sibling), 1u);
                    RotateLeft(parent);
                    break;
                }
            } else {
                auto sibling = Left(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateRight(parent);
                    sibling = Left(parent);
                }
                if (Color(Right(sibling)) == 1u &&
                    Color(Left(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    child = parent;
                    parent = Parent(child);
                } else {
                    if (Color(Left(sibling)) == 1u) {
                        SetColor(Right(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateLeft(sibling);
                        sibling = Left(parent);
                    }
                    SetColor(sibling, Color(parent));
                    SetColor(parent, 1u);
                    SetColor(Left(sibling), 1u);
                    RotateRight(parent);
                    break;
                }
            }
        }
        SetColor(child, 1u);
    }

private:
    std::uint32_t head_;
    IPalEventVolumnTreeEraseRuntime& runtime_;
    std::uint32_t nil_;
};

}  // namespace

std::uint32_t PalEventVolumnManager::EraseTreeNode(
    PalEventVolumnState& state, const std::uint32_t node,
    IPalEventVolumnTreeEraseRuntime& runtime) {
    const auto head = state.words[10u];
    TreeAccess tree(head, runtime);
    const auto nil = tree.Nil();
    auto successor = node;
    AdvanceTreeCursor(successor, runtime);
    const auto left = tree.Left(node);
    const auto right = tree.Right(node);
    auto removed = node;
    std::uint32_t child;
    if (left == nil)
        child = right;
    else if (right == nil)
        child = left;
    else {
        removed = tree.Minimum(right);
        child = tree.Right(removed);
    }
    const auto removed_black = tree.Color(removed) == 1u;

    std::uint32_t child_parent;
    if (removed != node) {
        tree.Set(left, 4u, removed);
        tree.Set(removed, 0u, left);
        if (removed == right) {
            child_parent = removed;
            tree.Set(child, 4u, removed);
        } else {
            child_parent = tree.Parent(removed);
            tree.Set(child, 4u, child_parent);
            tree.Set(child_parent, 0u, child);
            tree.Set(removed, 8u, right);
            tree.Set(right, 4u, removed);
        }
        const auto parent = tree.Parent(node);
        tree.ReplaceLink(node, removed);
        tree.Set(removed, 4u, parent);
        const auto color = tree.Color(removed);
        tree.SetColor(removed, tree.Color(node));
        tree.SetColor(node, color);
    } else {
        child_parent = tree.Parent(node);
        tree.Set(child, 4u, child_parent);
        tree.ReplaceLink(node, child);
        if (node == tree.Get(head)) {
            tree.Set(head, 0u, right == nil ? child_parent
                                            : tree.Minimum(child));
        }
        if (node == tree.Get(head, 8u)) {
            tree.Set(head, 8u, left == nil ? child_parent
                                           : tree.Maximum(child));
        }
    }

    if (removed_black)
        tree.RepairAfterErase(child, child_parent);
    runtime.FreeNode(node);
    --state.words[12u];
    return successor;
}

std::uint32_t PalEventVolumnManager::DestroyTreeSubtree(
    std::uint32_t root, IPalEventVolumnTreeSubtreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (root != nil) {
        (void)DestroyTreeSubtree(runtime.ReadWord(root + 8u), runtime);
        const auto left = runtime.ReadWord(root);
        runtime.FreeNode(root);
        root = left;
    }
    return nil;
}

std::uint32_t PalEventVolumnManager::AdvanceTreeIterator(
    std::uint32_t& cursor, IPalEventVolumnTreeSubtreeRuntime& runtime) {
    const auto old = cursor;
    const auto nil = runtime.SharedNil();
    auto right = runtime.ReadWord(cursor + 8u);
    if (right != nil) {
        while (runtime.ReadWord(right) != nil)
            right = runtime.ReadWord(right);
        cursor = right;
    } else {
        auto parent = runtime.ReadWord(cursor + 4u);
        while (cursor == runtime.ReadWord(parent + 8u)) {
            cursor = parent;
            parent = runtime.ReadWord(parent + 4u);
        }
        if (runtime.ReadWord(cursor + 8u) != parent)
            cursor = parent;
    }
    return old;
}

void PalEventVolumnManager::AdvanceTreeCursor(
    std::uint32_t& cursor, IPalEventVolumnTreeSubtreeRuntime& runtime) {
    (void)AdvanceTreeIterator(cursor, runtime);
}

std::uint32_t PalEventVolumnManager::InsertTreeNode(
    PalEventVolumnState& state, const std::uint32_t child,
    const std::uint32_t parent, const std::uint32_t key,
    const std::uint32_t value,
    IPalEventVolumnTreeNodeInsertRuntime& runtime) {
    const auto node = runtime.AllocateNode(24u);
    if (node == 0u) return 0u;
    const auto head = state.words[10u];
    TreeAccess tree(head, runtime);
    const auto nil = tree.Nil();
    tree.Set(node, 4u, parent);
    tree.SetColor(node, 0u);
    tree.Set(node, 0u, nil);
    tree.Set(node, 8u, nil);
    tree.Set(node, 12u, key);
    tree.Set(node, 16u, value);
    ++state.words[12u];

    if (parent == head || child != nil ||
        std::bit_cast<std::int32_t>(key) <
            std::bit_cast<std::int32_t>(tree.Get(parent, 12u))) {
        tree.Set(parent, 0u, node);
        if (parent == head) {
            tree.Set(head, 4u, node);
            tree.Set(head, 8u, node);
        } else if (parent == tree.Get(head)) {
            tree.Set(head, 0u, node);
        }
    } else {
        tree.Set(parent, 8u, node);
        if (parent == tree.Get(head, 8u))
            tree.Set(head, 8u, node);
    }

    auto current = node;
    while (current != tree.Root()) {
        const auto current_parent = tree.Parent(current);
        if (tree.Color(current_parent) != 0u) break;
        const auto grandparent = tree.Parent(current_parent);
        if (current_parent == tree.Left(grandparent)) {
            const auto uncle = tree.Right(grandparent);
            if (tree.Color(uncle) == 0u) {
                tree.SetColor(current_parent, 1u);
                tree.SetColor(uncle, 1u);
                tree.SetColor(grandparent, 0u);
                current = grandparent;
            } else {
                if (current == tree.Right(current_parent)) {
                    current = current_parent;
                    tree.RotateLeft(current);
                }
                const auto next_parent = tree.Parent(current);
                const auto next_grandparent = tree.Parent(next_parent);
                tree.SetColor(next_parent, 1u);
                tree.SetColor(next_grandparent, 0u);
                tree.RotateRight(next_grandparent);
            }
        } else {
            const auto uncle = tree.Left(grandparent);
            if (tree.Color(uncle) == 0u) {
                tree.SetColor(current_parent, 1u);
                tree.SetColor(uncle, 1u);
                tree.SetColor(grandparent, 0u);
                current = grandparent;
            } else {
                if (current == tree.Left(current_parent)) {
                    current = current_parent;
                    tree.RotateRight(current);
                }
                const auto next_parent = tree.Parent(current);
                const auto next_grandparent = tree.Parent(next_parent);
                tree.SetColor(next_parent, 1u);
                tree.SetColor(next_grandparent, 0u);
                tree.RotateLeft(next_grandparent);
            }
        }
    }
    tree.SetColor(tree.Root(), 1u);
    return node;
}

}  // namespace pal4
