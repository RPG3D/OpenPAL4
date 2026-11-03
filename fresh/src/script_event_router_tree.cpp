#include "pal4/script_event_router.h"

namespace pal4 {
namespace {

class Tree final {
public:
    Tree(const std::uint32_t router, IScriptEventRouterRuntime& runtime)
        : router_(router), head_(runtime.ReadWord(router + 4u)),
          nil_(runtime.ReadWord(ScriptEventRouter::kNilPointer)),
          runtime_(runtime) {}

    [[nodiscard]] std::uint32_t Head() const { return head_; }
    [[nodiscard]] std::uint32_t Nil() const { return nil_; }
    [[nodiscard]] std::uint32_t Left(std::uint32_t node) const {
        return runtime_.ReadWord(node);
    }
    [[nodiscard]] std::uint32_t Parent(std::uint32_t node) const {
        return runtime_.ReadWord(node + 4u);
    }
    [[nodiscard]] std::uint32_t Right(std::uint32_t node) const {
        return runtime_.ReadWord(node + 8u);
    }
    [[nodiscard]] std::uint32_t Color(std::uint32_t node) const {
        return runtime_.ReadWord(node + 28u);
    }
    [[nodiscard]] std::uint32_t Root() const { return Parent(head_); }
    void SetLeft(std::uint32_t node, std::uint32_t value) {
        runtime_.WriteWord(node, value);
    }
    void SetParent(std::uint32_t node, std::uint32_t value) {
        runtime_.WriteWord(node + 4u, value);
    }
    void SetRight(std::uint32_t node, std::uint32_t value) {
        runtime_.WriteWord(node + 8u, value);
    }
    void SetColor(std::uint32_t node, std::uint32_t value) {
        runtime_.WriteWord(node + 28u, value);
    }
    void ReplaceParentLink(std::uint32_t old_node,
                           std::uint32_t replacement) {
        const auto parent = Parent(old_node);
        if (Root() == old_node) SetParent(head_, replacement);
        else if (Left(parent) == old_node) SetLeft(parent, replacement);
        else SetRight(parent, replacement);
    }
    [[nodiscard]] std::uint32_t Minimum(std::uint32_t node) const {
        while (Left(node) != nil_) node = Left(node);
        return node;
    }
    [[nodiscard]] std::uint32_t Maximum(std::uint32_t node) const {
        while (Right(node) != nil_) node = Right(node);
        return node;
    }
    void RotateLeft(std::uint32_t node) {
        const auto pivot = Right(node);
        const auto moved = Left(pivot);
        SetRight(node, moved);
        if (moved != nil_) SetParent(moved, node);
        SetParent(pivot, Parent(node));
        ReplaceParentLink(node, pivot);
        SetLeft(pivot, node);
        SetParent(node, pivot);
    }
    void RotateRight(std::uint32_t node) {
        const auto pivot = Left(node);
        const auto moved = Right(pivot);
        SetLeft(node, moved);
        if (moved != nil_) SetParent(moved, node);
        SetParent(pivot, Parent(node));
        ReplaceParentLink(node, pivot);
        SetRight(pivot, node);
        SetParent(node, pivot);
    }
    void DecrementCount() {
        runtime_.WriteWord(router_ + 12u,
                           runtime_.ReadWord(router_ + 12u) - 1u);
    }

private:
    std::uint32_t router_;
    std::uint32_t head_;
    std::uint32_t nil_;
    IScriptEventRouterRuntime& runtime_;
};

std::uint32_t NextNodeImpl(std::uint32_t node,
                           IScriptEventRouterRuntime& runtime) {
    const auto nil = runtime.ReadWord(ScriptEventRouter::kNilPointer);
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
    if (runtime.ReadWord(node + 8u) != parent) node = parent;
    return node;
}

void RemoveEntry(const std::uint32_t list,
                 const std::uint32_t entry,
                 IScriptEventRouterRuntime& runtime) {
    const auto next = runtime.ReadWord(entry);
    const auto previous = runtime.ReadWord(entry + 4u);
    runtime.WriteWord(previous, next);
    runtime.WriteWord(next + 4u, previous);
    runtime.ReleaseOwnedCallback(entry + 8u);
    (void)runtime.Free(entry);
    runtime.WriteWord(list + 8u,
                      runtime.ReadWord(list + 8u) - 1u);
}

void CleanupList(const std::uint32_t node,
                 IScriptEventRouterRuntime& runtime) {
    const auto sentinel = runtime.ReadWord(node + 20u);
    auto entry = runtime.ReadWord(sentinel);
    while (entry != sentinel) {
        const auto next = runtime.ReadWord(entry);
        RemoveEntry(node + 16u, entry, runtime);
        entry = next;
    }
    (void)runtime.Free(sentinel);
    runtime.WriteWord(node + 20u, 0u);
    runtime.WriteWord(node + 24u, 0u);
}

void RestoreBlackHeight(Tree& tree, std::uint32_t child) {
    while (child != tree.Root() && tree.Color(child) == 1u) {
        const auto parent = tree.Parent(child);
        if (child == tree.Left(parent)) {
            auto sibling = tree.Right(parent);
            if (tree.Color(sibling) == 0u) {
                tree.SetColor(sibling, 1u);
                tree.SetColor(parent, 0u);
                tree.RotateLeft(parent);
                sibling = tree.Right(tree.Parent(child));
            }
            if (tree.Color(tree.Left(sibling)) == 1u &&
                tree.Color(tree.Right(sibling)) == 1u) {
                tree.SetColor(sibling, 0u);
                child = tree.Parent(child);
            } else {
                if (tree.Color(tree.Right(sibling)) == 1u) {
                    tree.SetColor(tree.Left(sibling), 1u);
                    tree.SetColor(sibling, 0u);
                    tree.RotateRight(sibling);
                    sibling = tree.Right(tree.Parent(child));
                }
                tree.SetColor(sibling, tree.Color(tree.Parent(child)));
                tree.SetColor(tree.Parent(child), 1u);
                tree.SetColor(tree.Right(sibling), 1u);
                tree.RotateLeft(tree.Parent(child));
                break;
            }
        } else {
            auto sibling = tree.Left(parent);
            if (tree.Color(sibling) == 0u) {
                tree.SetColor(sibling, 1u);
                tree.SetColor(parent, 0u);
                tree.RotateRight(parent);
                sibling = tree.Left(tree.Parent(child));
            }
            if (tree.Color(tree.Right(sibling)) == 1u &&
                tree.Color(tree.Left(sibling)) == 1u) {
                tree.SetColor(sibling, 0u);
                child = tree.Parent(child);
            } else {
                if (tree.Color(tree.Left(sibling)) == 1u) {
                    tree.SetColor(tree.Right(sibling), 1u);
                    tree.SetColor(sibling, 0u);
                    tree.RotateLeft(sibling);
                    sibling = tree.Left(tree.Parent(child));
                }
                tree.SetColor(sibling, tree.Color(tree.Parent(child)));
                tree.SetColor(tree.Parent(child), 1u);
                tree.SetColor(tree.Left(sibling), 1u);
                tree.RotateRight(tree.Parent(child));
                break;
            }
        }
    }
    tree.SetColor(child, 1u);
}

std::uint32_t EraseNodeImpl(const std::uint32_t router,
                            const std::uint32_t node,
                            IScriptEventRouterRuntime& runtime) {
    const auto next = NextNodeImpl(node, runtime);
    Tree tree(router, runtime);
    const auto left = tree.Left(node);
    const auto right = tree.Right(node);
    auto removed = node;
    std::uint32_t replacement;

    if (left == tree.Nil() || right == tree.Nil()) {
        replacement = left == tree.Nil() ? right : left;
        tree.SetParent(replacement, tree.Parent(node));
        tree.ReplaceParentLink(node, replacement);
        if (tree.Left(tree.Head()) == node) {
            tree.SetLeft(tree.Head(), right == tree.Nil()
                ? tree.Parent(node) : tree.Minimum(replacement));
        }
        if (tree.Right(tree.Head()) == node) {
            tree.SetRight(tree.Head(), left == tree.Nil()
                ? tree.Parent(node) : tree.Maximum(replacement));
        }
    } else {
        const auto successor = tree.Minimum(right);
        replacement = tree.Right(successor);
        tree.SetParent(left, successor);
        tree.SetLeft(successor, left);
        if (successor == right) {
            tree.SetParent(replacement, successor);
        } else {
            const auto successor_parent = tree.Parent(successor);
            tree.SetParent(replacement, successor_parent);
            tree.SetLeft(successor_parent, replacement);
            tree.SetRight(successor, right);
            tree.SetParent(right, successor);
        }
        tree.ReplaceParentLink(node, successor);
        tree.SetParent(successor, tree.Parent(node));
        const auto successor_color = tree.Color(successor);
        tree.SetColor(successor, tree.Color(node));
        tree.SetColor(node, successor_color);
    }

    if (tree.Color(removed) == 1u)
        RestoreBlackHeight(tree, replacement);
    CleanupList(removed, runtime);
    (void)runtime.Free(removed);
    tree.DecrementCount();
    return next;
}

}  // namespace

std::uint32_t ScriptEventRouterTree::EraseToOutput(
    const std::uint32_t router, const std::uint32_t output,
    const std::uint32_t node, IScriptEventRouterRuntime& runtime) {
    const auto next = EraseNodeImpl(router, node, runtime);
    runtime.WriteWord(output, next);
    return output;
}

std::uint32_t ScriptEventRouterTree::EraseNode(
    const std::uint32_t router, const std::uint32_t node,
    IScriptEventRouterRuntime& runtime) {
    return EraseNodeImpl(router, node, runtime);
}

std::uint32_t ScriptEventRouterTree::EraseRecursive(
    std::uint32_t root, IScriptEventRouterRuntime& runtime) {
    const auto nil = runtime.ReadWord(ScriptEventRouter::kNilPointer);
    while (root != nil) {
        (void)EraseRecursive(runtime.ReadWord(root + 8u), runtime);
        const auto left = runtime.ReadWord(root);
        CleanupList(root, runtime);
        (void)runtime.Free(root);
        root = left;
    }
    return nil;
}

std::uint32_t ScriptEventRouterTree::GetEvent(
    const std::uint32_t entry) {
    return entry;
}

std::uint32_t ScriptEventRouterTree::NextEvent(
    const std::uint32_t iterator_slot,
    IScriptEventRouterRuntime& runtime) {
    const auto nil = runtime.ReadWord(ScriptEventRouter::kNilPointer);
    auto node = runtime.ReadWord(iterator_slot);
    auto right = runtime.ReadWord(node + 8u);
    if (right != nil) {
        auto scan = runtime.ReadWord(right);
        while (scan != nil) {
            right = scan;
            scan = runtime.ReadWord(right);
        }
        runtime.WriteWord(iterator_slot, right);
        return scan;
    }
    auto parent = runtime.ReadWord(node + 4u);
    while (node == runtime.ReadWord(parent + 8u)) {
        runtime.WriteWord(iterator_slot, parent);
        node = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    node = runtime.ReadWord(iterator_slot);
    if (runtime.ReadWord(node + 8u) != parent)
        runtime.WriteWord(iterator_slot, parent);
    return parent;
}

std::uint32_t ScriptEventRouterTree::NextNode(
    const std::uint32_t node,
    IScriptEventRouterRuntime& runtime) {
    return NextNodeImpl(node, runtime);
}

std::uint32_t ScriptEventRouterTree::RemoveEvent(
    const std::uint32_t list, const std::uint32_t output,
    const std::uint32_t entry, IScriptEventRouterRuntime& runtime) {
    const auto next = runtime.ReadWord(entry);
    RemoveEntry(list, entry, runtime);
    runtime.WriteWord(output, next);
    return output;
}

}  // namespace pal4
