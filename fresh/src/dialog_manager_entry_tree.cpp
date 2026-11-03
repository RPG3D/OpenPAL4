#include "pal4/dialog_manager_entry_tree.h"

namespace pal4 {
namespace {

std::uint32_t Successor(std::uint32_t node,
                        IDialogManagerEntryTreeRuntime& runtime) {
    const auto nil = runtime.NilNode();
    auto right = runtime.NodeWord(node, 8);
    if (right != nil) {
        while (runtime.NodeWord(right, 0) != nil)
            right = runtime.NodeWord(right, 0);
        return right;
    }
    auto parent = runtime.NodeWord(node, 4);
    while (node == runtime.NodeWord(parent, 8)) {
        node = parent;
        parent = runtime.NodeWord(parent, 4);
    }
    return runtime.NodeWord(node, 8) == parent ? node : parent;
}

void RepairAfterErase(DialogManagerState& state, const std::uint32_t head,
                      std::uint32_t child,
                      IDialogManagerEntryTreeEraseRuntime& runtime) {
    while (child != runtime.NodeWord(head, 4) &&
           runtime.NodeWord(child, 16) == 1) {
        auto parent = runtime.NodeWord(child, 4);
        if (child == runtime.NodeWord(parent, 0)) {
            auto sibling = runtime.NodeWord(parent, 8);
            if (runtime.NodeWord(sibling, 16) == 0) {
                runtime.SetNodeWord(sibling, 16, 1);
                runtime.SetNodeWord(parent, 16, 0);
                (void)DialogManagerEntryTree::RotateLeft(state, parent, runtime);
                parent = runtime.NodeWord(child, 4);
                sibling = runtime.NodeWord(parent, 8);
            }
            const auto near_child = runtime.NodeWord(sibling, 0);
            const auto far_child = runtime.NodeWord(sibling, 8);
            if (runtime.NodeWord(near_child, 16) == 1 &&
                runtime.NodeWord(far_child, 16) == 1) {
                runtime.SetNodeWord(sibling, 16, 0);
                child = parent;
            } else {
                if (runtime.NodeWord(far_child, 16) == 1) {
                    runtime.SetNodeWord(near_child, 16, 1);
                    runtime.SetNodeWord(sibling, 16, 0);
                    (void)DialogManagerEntryTree::RotateRight(state, sibling, runtime);
                    sibling = runtime.NodeWord(parent, 8);
                }
                runtime.SetNodeWord(sibling, 16,
                                    runtime.NodeWord(parent, 16));
                runtime.SetNodeWord(parent, 16, 1);
                runtime.SetNodeWord(runtime.NodeWord(sibling, 8), 16, 1);
                (void)DialogManagerEntryTree::RotateLeft(state, parent, runtime);
                break;
            }
        } else {
            auto sibling = runtime.NodeWord(parent, 0);
            if (runtime.NodeWord(sibling, 16) == 0) {
                runtime.SetNodeWord(sibling, 16, 1);
                runtime.SetNodeWord(parent, 16, 0);
                (void)DialogManagerEntryTree::RotateRight(state, parent, runtime);
                parent = runtime.NodeWord(child, 4);
                sibling = runtime.NodeWord(parent, 0);
            }
            const auto near_child = runtime.NodeWord(sibling, 8);
            const auto far_child = runtime.NodeWord(sibling, 0);
            if (runtime.NodeWord(near_child, 16) == 1 &&
                runtime.NodeWord(far_child, 16) == 1) {
                runtime.SetNodeWord(sibling, 16, 0);
                child = parent;
            } else {
                if (runtime.NodeWord(far_child, 16) == 1) {
                    runtime.SetNodeWord(near_child, 16, 1);
                    runtime.SetNodeWord(sibling, 16, 0);
                    (void)DialogManagerEntryTree::RotateLeft(state, sibling, runtime);
                    sibling = runtime.NodeWord(parent, 0);
                }
                runtime.SetNodeWord(sibling, 16,
                                    runtime.NodeWord(parent, 16));
                runtime.SetNodeWord(parent, 16, 1);
                runtime.SetNodeWord(runtime.NodeWord(sibling, 0), 16, 1);
                (void)DialogManagerEntryTree::RotateRight(state, parent, runtime);
                break;
            }
        }
    }
    runtime.SetNodeWord(child, 16, 1);
}

}  // namespace

DialogEntryInsertResult DialogManagerEntryTree::Insert(
    DialogManagerState& state, const std::uint32_t key,
    IDialogManagerEntryTreeRuntime& runtime) {
    const auto nil = runtime.NilNode();
    const auto head = state.Word(4);
    auto parent = head;
    auto leaf = runtime.NodeWord(head, 4);
    bool less = true;
    while (leaf != nil) {
        parent = leaf;
        less = key < runtime.NodeWord(leaf, 12);
        leaf = runtime.NodeWord(leaf, less ? 0 : 8);
    }

    if (state.Byte(8) == 0) {
        auto candidate = parent;
        if (less) {
            if (parent == runtime.NodeWord(head, 0)) {
                std::uint32_t inserted = 0;
                (void)InsertAt(state, inserted, leaf, parent, key, runtime);
                return {inserted, true};
            }
            (void)Predecessor(candidate, runtime);
        }
        if (runtime.NodeWord(candidate, 12) >= key)
            return {candidate, false};
        std::uint32_t inserted = 0;
        (void)InsertAt(state, inserted, leaf, parent, key, runtime);
        return {inserted, true};
    }

    std::uint32_t inserted = 0;
    (void)InsertAt(state, inserted, leaf, parent, key, runtime);
    return {inserted, true};
}

std::uint32_t& DialogManagerEntryTree::InsertAt(
    DialogManagerState& state, std::uint32_t& output,
    const std::uint32_t leaf, const std::uint32_t parent,
    const std::uint32_t key, IDialogManagerEntryTreeRuntime& runtime) {
    const auto nil = runtime.NilNode();
    const auto head = state.Word(4);
    const auto inserted = CreateNode(parent, 0, runtime);
    runtime.SetNodeWord(inserted, 0, nil);
    runtime.SetNodeWord(inserted, 8, nil);
    runtime.SetNodeWord(inserted, 12, key);
    state.SetWord(12, state.Word(12) + 1);
    if (parent == head || leaf != nil || key < runtime.NodeWord(parent, 12)) {
        runtime.SetNodeWord(parent, 0, inserted);
        if (parent == head) {
            runtime.SetNodeWord(head, 4, inserted);
            runtime.SetNodeWord(head, 8, inserted);
        } else if (parent == runtime.NodeWord(head, 0)) {
            runtime.SetNodeWord(head, 0, inserted);
        }
    } else {
        runtime.SetNodeWord(parent, 8, inserted);
        if (parent == runtime.NodeWord(head, 8))
            runtime.SetNodeWord(head, 8, inserted);
    }

    auto node = inserted;
    while (node != runtime.NodeWord(head, 4)) {
        auto parent_node = runtime.NodeWord(node, 4);
        if (runtime.NodeWord(parent_node, 16) != 0) break;
        const auto grandparent = runtime.NodeWord(parent_node, 4);
        if (parent_node == runtime.NodeWord(grandparent, 0)) {
            const auto uncle = runtime.NodeWord(grandparent, 8);
            if (runtime.NodeWord(uncle, 16) == 0) {
                runtime.SetNodeWord(parent_node, 16, 1);
                runtime.SetNodeWord(uncle, 16, 1);
                runtime.SetNodeWord(grandparent, 16, 0);
                node = grandparent;
            } else {
                if (node == runtime.NodeWord(parent_node, 8)) {
                    node = parent_node;
                    (void)RotateLeft(state, node, runtime);
                }
                parent_node = runtime.NodeWord(node, 4);
                const auto grand = runtime.NodeWord(parent_node, 4);
                runtime.SetNodeWord(parent_node, 16, 1);
                runtime.SetNodeWord(grand, 16, 0);
                (void)RotateRight(state, grand, runtime);
            }
        } else {
            const auto uncle = runtime.NodeWord(grandparent, 0);
            if (runtime.NodeWord(uncle, 16) == 0) {
                runtime.SetNodeWord(parent_node, 16, 1);
                runtime.SetNodeWord(uncle, 16, 1);
                runtime.SetNodeWord(grandparent, 16, 0);
                node = grandparent;
            } else {
                if (node == runtime.NodeWord(parent_node, 0)) {
                    node = parent_node;
                    (void)RotateRight(state, node, runtime);
                }
                parent_node = runtime.NodeWord(node, 4);
                const auto grand = runtime.NodeWord(parent_node, 4);
                runtime.SetNodeWord(parent_node, 16, 1);
                runtime.SetNodeWord(grand, 16, 0);
                (void)RotateLeft(state, grand, runtime);
            }
        }
    }
    runtime.SetNodeWord(runtime.NodeWord(head, 4), 16, 1);
    output = inserted;
    return output;
}

std::uint32_t DialogManagerEntryTree::RotateLeft(
    DialogManagerState& state, const std::uint32_t node,
    IDialogManagerEntryTreeRuntime& runtime) {
    const auto head = state.Word(4);
    const auto pivot = runtime.NodeWord(node, 8);
    const auto moved = runtime.NodeWord(pivot, 0);
    runtime.SetNodeWord(node, 8, moved);
    if (moved != runtime.NilNode()) runtime.SetNodeWord(moved, 4, node);
    const auto parent = runtime.NodeWord(node, 4);
    runtime.SetNodeWord(pivot, 4, parent);
    if (node == runtime.NodeWord(head, 4))
        runtime.SetNodeWord(head, 4, pivot);
    else if (node == runtime.NodeWord(parent, 0))
        runtime.SetNodeWord(parent, 0, pivot);
    else
        runtime.SetNodeWord(parent, 8, pivot);
    runtime.SetNodeWord(pivot, 0, node);
    runtime.SetNodeWord(node, 4, pivot);
    return pivot;
}

std::uint32_t DialogManagerEntryTree::RotateRight(
    DialogManagerState& state, const std::uint32_t node,
    IDialogManagerEntryTreeRuntime& runtime) {
    const auto head = state.Word(4);
    const auto pivot = runtime.NodeWord(node, 0);
    const auto moved = runtime.NodeWord(pivot, 8);
    runtime.SetNodeWord(node, 0, moved);
    if (moved != runtime.NilNode()) runtime.SetNodeWord(moved, 4, node);
    const auto parent = runtime.NodeWord(node, 4);
    runtime.SetNodeWord(pivot, 4, parent);
    if (node == runtime.NodeWord(head, 4))
        runtime.SetNodeWord(head, 4, pivot);
    else if (node == runtime.NodeWord(parent, 8))
        runtime.SetNodeWord(parent, 8, pivot);
    else
        runtime.SetNodeWord(parent, 0, pivot);
    runtime.SetNodeWord(pivot, 8, node);
    runtime.SetNodeWord(node, 4, pivot);
    return pivot;
}

std::uint32_t DialogManagerEntryTree::CreateNode(
    const std::uint32_t parent, const std::uint32_t color,
    IDialogManagerEntryTreeRuntime& runtime) {
    const auto node = runtime.AllocateNode(0x14);
    runtime.SetNodeWord(node, 4, parent);
    runtime.SetNodeWord(node, 16, color);
    return node;
}

std::uint32_t DialogManagerEntryTree::Predecessor(
    std::uint32_t& node, IDialogManagerEntryTreeRuntime& runtime) {
    const auto parent = runtime.NodeWord(node, 4);
    if (runtime.NodeWord(node, 16) == 0 &&
        runtime.NodeWord(parent, 4) == node) {
        node = runtime.NodeWord(node, 8);
        return node;
    }
    auto left = runtime.NodeWord(node, 0);
    if (left != runtime.NilNode()) {
        auto right = runtime.NodeWord(left, 8);
        while (right != runtime.NilNode()) {
            left = right;
            right = runtime.NodeWord(left, 8);
        }
        node = left;
        return right;
    }
    auto ancestor = parent;
    while (node == runtime.NodeWord(ancestor, 0)) {
        node = ancestor;
        ancestor = runtime.NodeWord(ancestor, 4);
    }
    node = ancestor;
    return ancestor;
}

std::uint32_t& DialogManagerEntryTree::Erase(
    DialogManagerState& state, std::uint32_t& output,
    const std::uint32_t node, IDialogManagerEntryTreeEraseRuntime& runtime) {
    const auto nil = runtime.NilNode();
    const auto head = state.Word(4);
    const auto successor = Successor(node, runtime);
    const auto left = runtime.NodeWord(node, 0);
    const auto right = runtime.NodeWord(node, 8);
    auto removed = node;
    std::uint32_t child;
    if (left == nil) {
        child = right;
    } else if (right == nil) {
        child = left;
    } else {
        removed = right;
        while (runtime.NodeWord(removed, 0) != nil)
            removed = runtime.NodeWord(removed, 0);
        child = runtime.NodeWord(removed, 8);
    }

    if (removed != node) {
        runtime.SetNodeWord(left, 4, removed);
        runtime.SetNodeWord(removed, 0, left);
        if (removed == right) {
            runtime.SetNodeWord(child, 4, removed);
        } else {
            const auto old_parent = runtime.NodeWord(removed, 4);
            runtime.SetNodeWord(child, 4, old_parent);
            runtime.SetNodeWord(old_parent, 0, child);
            runtime.SetNodeWord(removed, 8, right);
            runtime.SetNodeWord(right, 4, removed);
        }
        const auto parent = runtime.NodeWord(node, 4);
        if (node == runtime.NodeWord(head, 4))
            runtime.SetNodeWord(head, 4, removed);
        else if (node == runtime.NodeWord(parent, 0))
            runtime.SetNodeWord(parent, 0, removed);
        else
            runtime.SetNodeWord(parent, 8, removed);
        runtime.SetNodeWord(removed, 4, parent);
        const auto color = runtime.NodeWord(removed, 16);
        runtime.SetNodeWord(removed, 16, runtime.NodeWord(node, 16));
        runtime.SetNodeWord(node, 16, color);
        removed = node;
    } else {
        const auto parent = runtime.NodeWord(node, 4);
        runtime.SetNodeWord(child, 4, parent);
        if (node == runtime.NodeWord(head, 4))
            runtime.SetNodeWord(head, 4, child);
        else if (node == runtime.NodeWord(parent, 0))
            runtime.SetNodeWord(parent, 0, child);
        else
            runtime.SetNodeWord(parent, 8, child);

        if (node == runtime.NodeWord(head, 0)) {
            if (right == nil) {
                runtime.SetNodeWord(head, 0, parent);
            } else {
                auto minimum = child;
                while (runtime.NodeWord(minimum, 0) != nil)
                    minimum = runtime.NodeWord(minimum, 0);
                runtime.SetNodeWord(head, 0, minimum);
            }
        }
        if (node == runtime.NodeWord(head, 8)) {
            if (left == nil) {
                runtime.SetNodeWord(head, 8, parent);
            } else {
                auto maximum = child;
                while (runtime.NodeWord(maximum, 8) != nil)
                    maximum = runtime.NodeWord(maximum, 8);
                runtime.SetNodeWord(head, 8, maximum);
            }
        }
    }

    if (runtime.NodeWord(removed, 16) == 1)
        RepairAfterErase(state, head, child, runtime);
    runtime.FreeNode(removed);
    state.SetWord(12, state.Word(12) - 1);
    output = successor;
    return output;
}

}  // namespace pal4
