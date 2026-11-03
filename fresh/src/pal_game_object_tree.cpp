#include "pal4/pal_game_object_tree.h"
#include "pal4/ui_tree_pair_data.h"

namespace pal4 {
namespace {

std::uint32_t Predecessor(std::uint32_t node,
                          IPalGameObjectNameInsertRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto left = runtime.ReadWord(node);
    if (left != nil) {
        while (runtime.ReadWord(left + 8u) != nil)
            left = runtime.ReadWord(left + 8u);
        return left;
    }
    auto parent = runtime.ReadWord(node + 4u);
    while (node == runtime.ReadWord(parent)) {
        node = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    return parent;
}

}  // namespace

std::uint32_t PalGameObjectTree::InsertName(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t key,
    IPalGameObjectNameInsertRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil();
    auto parent = head;
    auto node = runtime.ReadWord(head + 4u);
    bool left = true;
    while (node != nil) {
        parent = node;
        left = runtime.CompareText(key, node + 12u) < 0;
        node = runtime.ReadWord(node + (left ? 0u : 8u));
    }

    bool insert = runtime.ReadByte(tree + 8u) != 0u;
    if (!insert) {
        auto candidate = parent;
        if (left) {
            if (parent == runtime.ReadWord(head)) {
                insert = true;
            } else {
                candidate = Predecessor(parent, runtime);
            }
        }
        if (!insert)
            insert = runtime.CompareText(candidate + 12u, key) < 0;
        if (!insert) parent = candidate;
    }

    if (insert) {
        static_cast<void>(InsertNameNode(
            tree, output, node, parent, key, runtime));
        runtime.WriteByte(output + 4u, 1u);
    } else {
        runtime.WriteWord(output, parent);
        runtime.WriteByte(output + 4u, 0u);
    }
    return output;
}

std::uint32_t PalGameObjectTree::RemoveName(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t node, IUiRedBlackTreeRemoveRuntime& runtime) {
    return UiRedBlackTreeRemove::Remove(tree, output, node, runtime);
}

std::uint32_t PalGameObjectTree::NextName(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::Next(cursor, runtime);
}

std::uint32_t PalGameObjectTree::LowerBoundName(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t query,
    IUiRedBlackTreeLookupRuntime& runtime) {
    return UiRedBlackTreeLookup::LowerBound(tree, output, query, runtime);
}

std::uint32_t PalGameObjectTree::ClearNameNodes(
    const std::uint32_t node,
    IUiRedBlackTreeStorageRuntime& runtime) {
    return UiRedBlackTreeStorage::FreeNodes(node, runtime);
}

std::uint32_t PalGameObjectTree::InsertNameNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t key,
    IUiRedBlackTreeInsertRuntime& runtime) {
    return UiRedBlackTreeInsert::Insert(
        tree, output, child, parent, key, runtime);
}

std::uint32_t PalGameObjectTree::RotateNameLeft(
    const std::uint32_t tree, const std::uint32_t node,
    IUiRedBlackTreeLinkRuntime& runtime) {
    return UiRedBlackTreeRotation::RotateLeft(tree, node, runtime);
}

std::uint32_t PalGameObjectTree::LeftmostName(
    const std::uint32_t slot, IUiRedBlackTreeLookupRuntime& runtime) {
    return UiRedBlackTreeLookup::LeftmostFromSlot(slot, runtime);
}

std::uint32_t PalGameObjectTree::RotateNameRight(
    const std::uint32_t tree, const std::uint32_t node,
    IUiRedBlackTreeLinkRuntime& runtime) {
    return UiRedBlackTreeRotation::RotateRight(tree, node, runtime);
}

std::uint32_t PalGameObjectTree::AdvanceName(
    const std::uint32_t cursor, const std::uint32_t output,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::AdvanceAndReturnPrevious(
        cursor, output, runtime);
}

std::uint32_t PalGameObjectTree::FindFactory(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t key, IUiRedBlackTreeLinkRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    while (node != runtime.SharedNil()) {
        const auto node_key = static_cast<std::int32_t>(
            runtime.ReadWord(node + 12u));
        if (node_key >= static_cast<std::int32_t>(runtime.ReadWord(key))) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    if (candidate == head ||
        static_cast<std::int32_t>(runtime.ReadWord(key)) <
            static_cast<std::int32_t>(runtime.ReadWord(candidate + 12u)))
        candidate = head;
    runtime.WriteWord(output, candidate);
    return output;
}

std::uint32_t PalGameObjectTree::PreviousName(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::Previous(cursor, runtime);
}

std::uint32_t PalGameObjectTree::RemoveFactory(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t node,
    IUiRedBlackTreeRemoveRuntime& runtime) {
    return UiRedBlackTreeRemove::RemoveFactory(tree, output, node, runtime);
}

std::uint32_t PalGameObjectTree::ClearFactoryNodes(
    std::uint32_t node, IPalGameObjectFactoryClearRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (node != nil) {
        static_cast<void>(ClearFactoryNodes(runtime.ReadWord(node + 8u),
                                            runtime));
        const auto left = runtime.ReadWord(node);
        runtime.FreeMemory(node);
        node = left;
    }
    return nil;
}

std::uint32_t PalGameObjectTree::AdvanceFactory(
    const std::uint32_t cursor, const std::uint32_t output,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::AdvanceAndReturnPrevious(
        cursor, output, runtime);
}

std::uint32_t PalGameObjectTree::NextFactory(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::Next(cursor, runtime);
}

std::uint32_t PalGameObjectTree::InsertFactoryNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t value,
    IPalGameObjectFactoryInsertRuntime& runtime) {
    const auto node = runtime.AllocateMemory(0x18u);
    const auto nil = runtime.SharedNil();
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 20u, 0u);
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    static_cast<void>(UiTreePairData::Copy(node + 12u, value, runtime));

    const auto head = runtime.ReadWord(tree + 4u);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) + 1u);
    if (parent == head || child != nil ||
        static_cast<std::int32_t>(runtime.ReadWord(value)) <
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
    while (current != runtime.ReadWord(head + 4u) &&
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
                    static_cast<void>(UiRedBlackTreeRotation::RotateLeft(
                        tree, current, runtime));
                }
                const auto new_upper = runtime.ReadWord(current + 4u);
                const auto new_root = runtime.ReadWord(new_upper + 4u);
                runtime.WriteWord(new_upper + 20u, 1u);
                runtime.WriteWord(new_root + 20u, 0u);
                static_cast<void>(UiRedBlackTreeRotation::RotateRight(
                    tree, new_root, runtime));
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
                    static_cast<void>(UiRedBlackTreeRotation::RotateRight(
                        tree, current, runtime));
                }
                const auto new_upper = runtime.ReadWord(current + 4u);
                const auto new_root = runtime.ReadWord(new_upper + 4u);
                runtime.WriteWord(new_upper + 20u, 1u);
                runtime.WriteWord(new_root + 20u, 0u);
                static_cast<void>(UiRedBlackTreeRotation::RotateLeft(
                    tree, new_root, runtime));
            }
        }
    }
    runtime.WriteWord(runtime.ReadWord(head + 4u) + 20u, 1u);
    runtime.WriteWord(output, node);
    return output;
}

std::uint32_t PalGameObjectTree::PreviousFactory(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return UiRedBlackTreeIterator::PreviousFactory(cursor, runtime);
}

}  // namespace pal4
