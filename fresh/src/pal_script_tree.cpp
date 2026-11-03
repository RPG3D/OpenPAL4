#include "pal4/pal_script_tree.h"

namespace pal4 {

PkScriptNodeTag& PalScriptTree::CopyPkNodeTag(
    PkScriptNodeTag& output, const std::uint32_t value,
    const std::uint8_t flag) {
    output.value = value;
    output.flag = flag;
    return output;
}

std::uint32_t PalScriptTree::RemoveNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t node, IUiRedBlackTreeRemoveRuntime& runtime) {
    return UiRedBlackTreeRemove::RemoveFactory(
        tree, output, node, runtime);
}

std::uint32_t PalScriptTree::RemovePkNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t node, IUiRedBlackTreeRemoveRuntime& runtime) {
    return UiRedBlackTreeRemove::RemoveFactory(
        tree, output, node, runtime);
}

std::uint32_t PalScriptTree::NextNode(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::Next(cursor, runtime);
}

std::uint32_t PalScriptTree::NextPkNode(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::Next(cursor, runtime);
}

std::uint32_t PalScriptTree::FreeNodesRecursive(
    std::uint32_t root, IUiRedBlackTreeRemoveRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (root != nil) {
        (void)FreeNodesRecursive(runtime.ReadWord(root + 8u), runtime);
        const auto left = runtime.ReadWord(root);
        runtime.FreeMemory(root);
        root = left;
    }
    return nil;
}

std::uint32_t PalScriptTree::FreePkNodesRecursive(
    const std::uint32_t root, IUiRedBlackTreeRemoveRuntime& runtime) {
    return FreeNodesRecursive(root, runtime);
}

std::uint32_t PalScriptTree::InsertNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t value,
    IPalGameObjectFactoryInsertRuntime& runtime) {
    return PalGameObjectTree::InsertFactoryNode(
        tree, output, child, parent, value, runtime);
}

std::uint32_t PalScriptTree::InsertPkNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t value, IPalScriptTreeInsertRuntime& runtime) {
    const auto node = runtime.AllocateMemory(24u);
    const auto nil = runtime.SharedNil();
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 8u, nil);
    runtime.WriteWord(node + 12u, runtime.ReadWord(value));
    runtime.WriteWord(node + 16u, runtime.ReadWord(value + 4u));
    runtime.WriteWord(node + 20u, 0u);

    const auto header = runtime.ReadWord(tree + 4u);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) + 1u);
    if (parent == header || child != nil ||
        runtime.ReadWord(value) < runtime.ReadWord(parent + 12u)) {
        runtime.WriteWord(parent, node);
        if (parent == header) {
            runtime.WriteWord(header + 4u, node);
            runtime.WriteWord(header + 8u, node);
        } else if (parent == runtime.ReadWord(header)) {
            runtime.WriteWord(header, node);
        }
    } else {
        runtime.WriteWord(parent + 8u, node);
        if (parent == runtime.ReadWord(header + 8u))
            runtime.WriteWord(header + 8u, node);
    }

    auto current = node;
    while (current != runtime.ReadWord(header + 4u) &&
           runtime.ReadWord(runtime.ReadWord(current + 4u) + 20u) == 0u) {
        const auto upper = runtime.ReadWord(current + 4u);
        const auto root = runtime.ReadWord(upper + 4u);
        if (upper == runtime.ReadWord(root)) {
            const auto uncle = runtime.ReadWord(root + 8u);
            if (runtime.ReadWord(uncle + 20u) == 0u) {
                runtime.WriteWord(upper + 20u, 1u);
                runtime.WriteWord(uncle + 20u, 1u);
                runtime.WriteWord(root + 20u, 0u);
                current = root;
            } else {
                if (current == runtime.ReadWord(upper + 8u)) {
                    current = upper;
                    (void)UiRedBlackTreeRotation::RotateLeft(tree, current,
                                                              runtime);
                }
                const auto parent_node = runtime.ReadWord(current + 4u);
                const auto grandparent = runtime.ReadWord(parent_node + 4u);
                runtime.WriteWord(parent_node + 20u, 1u);
                runtime.WriteWord(grandparent + 20u, 0u);
                (void)UiRedBlackTreeRotation::RotateRight(tree, grandparent,
                                                           runtime);
            }
        } else {
            const auto uncle = runtime.ReadWord(root);
            if (runtime.ReadWord(uncle + 20u) == 0u) {
                runtime.WriteWord(upper + 20u, 1u);
                runtime.WriteWord(uncle + 20u, 1u);
                runtime.WriteWord(root + 20u, 0u);
                current = root;
            } else {
                if (current == runtime.ReadWord(upper)) {
                    current = upper;
                    (void)UiRedBlackTreeRotation::RotateRight(tree, current,
                                                               runtime);
                }
                const auto parent_node = runtime.ReadWord(current + 4u);
                const auto grandparent = runtime.ReadWord(parent_node + 4u);
                runtime.WriteWord(parent_node + 20u, 1u);
                runtime.WriteWord(grandparent + 20u, 0u);
                (void)UiRedBlackTreeRotation::RotateLeft(tree, grandparent,
                                                          runtime);
            }
        }
    }
    runtime.WriteWord(runtime.ReadWord(header + 4u) + 20u, 1u);
    runtime.WriteWord(output, node);
    return output;
}

std::uint32_t PalScriptTree::LowerBound(
    const std::uint32_t tree, const std::uint32_t key,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    auto candidate = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(candidate + 4u);
    const auto nil = runtime.SharedNil();
    while (node != nil) {
        if (static_cast<std::int32_t>(runtime.ReadWord(node + 12u)) >=
            static_cast<std::int32_t>(key)) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return candidate;
}

std::uint32_t PalScriptTree::LowerBoundPk(
    const std::uint32_t tree, const std::uint32_t key,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    auto candidate = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(candidate + 4u);
    const auto nil = runtime.SharedNil();
    while (node != nil) {
        if (runtime.ReadWord(node + 12u) >= key) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return candidate;
}

std::uint32_t PalScriptTree::PreviousNode(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::PreviousFactory(cursor, runtime);
}

std::uint32_t PalScriptTree::PreviousPkNode(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::PreviousFactory(cursor, runtime);
}

std::uint32_t PalScriptTree::ClearRange(
    const std::uint32_t tree, const std::uint32_t output,
    std::uint32_t first, const std::uint32_t last,
    IUiRedBlackTreeRemoveRuntime& runtime) {
    const auto header = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil();
    if (runtime.ReadWord(tree + 12u) != 0u &&
        first == runtime.ReadWord(header) && last == header) {
        auto node = runtime.ReadWord(header + 4u);
        while (node != nil) {
            (void)FreeNodesRecursive(runtime.ReadWord(node + 8u), runtime);
            const auto left = runtime.ReadWord(node);
            runtime.FreeMemory(node);
            node = left;
        }
        runtime.WriteWord(header + 4u, nil);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(header, header);
        runtime.WriteWord(header + 8u, header);
        runtime.WriteWord(output, header);
        return output;
    }

    while (first != last) {
        const auto current = first;
        const auto right = runtime.ReadWord(first + 8u);
        if (right != nil) {
            first = LeftmostNode(right, runtime);
        } else {
            auto parent = runtime.ReadWord(first + 4u);
            while (first == runtime.ReadWord(parent + 8u)) {
                first = parent;
                parent = runtime.ReadWord(parent + 4u);
            }
            if (runtime.ReadWord(first + 8u) != parent) first = parent;
        }
        (void)RemoveNode(tree, output, current, runtime);
    }
    runtime.WriteWord(output, first);
    return output;
}

std::uint32_t PalScriptTree::LeftmostNode(
    std::uint32_t root, IUiRedBlackTreeRemoveRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto left = runtime.ReadWord(root);
    while (left != nil) {
        root = left;
        left = runtime.ReadWord(root);
    }
    return root;
}

std::uint32_t PalScriptTree::ClearAll(
    const std::uint32_t tree, std::uint32_t& instance_count,
    std::uint32_t& shared_nil, IPalScriptTreeClearRuntime& runtime) {
    const auto header = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(header);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        (void)FreeNodesRecursive(runtime.ReadWord(header + 4u), runtime);
        runtime.WriteWord(header + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(header, header);
        runtime.WriteWord(header + 8u, header);
        runtime.FirstElement(tree);
    } else {
        while (node != header) {
            const auto current = node;
            const auto cursor = runtime.TemporaryCursor();
            runtime.WriteWord(cursor, node);
            (void)NextNode(cursor, runtime);
            node = runtime.ReadWord(cursor);
            (void)RemoveNode(tree, cursor, current, runtime);
        }
    }

    (void)runtime.ReleaseMemory(header);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    --instance_count;
    if (instance_count != 0u) return instance_count;
    const auto nil = shared_nil;
    shared_nil = 0u;
    return nil != 0u ? runtime.ReleaseMemory(nil) : 0u;
}

}  // namespace pal4
