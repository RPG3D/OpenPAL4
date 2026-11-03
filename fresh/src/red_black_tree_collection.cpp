#include "pal4/red_black_tree_collection.h"

namespace pal4 {

std::uint32_t RedBlackTreeCollection::Destroy(
    IRedBlackTreeCollectionRuntime& runtime) const {
    const auto header = runtime.ReadDword(address_ + 4u);
    auto node = runtime.ReadDword(header);
    if (runtime.ReadDword(address_ + 12u) != 0u) {
        runtime.FreeNodes(address_, runtime.ReadDword(header + 4u));
        runtime.WriteDword(header + 4u, runtime.SharedNil());
        runtime.WriteDword(address_ + 12u, 0u);
        runtime.WriteDword(header, header);
        runtime.WriteDword(header + 8u, header);
    } else if (node != header) {
        do {
            const auto current = node;
            runtime.NextNode(node);
            runtime.DeleteNode(address_, current);
        } while (node != header);
    }
    (void)runtime.FreeMemory(header);
    runtime.WriteDword(address_ + 4u, 0u);
    runtime.WriteDword(address_ + 12u, 0u);
    const auto remaining = runtime.SharedNilUsers() - 1u;
    runtime.SetSharedNilUsers(remaining);
    if (remaining != 0u) return remaining;
    const auto nil = runtime.SharedNil();
    runtime.SetSharedNil(0u);
    return nil == 0u ? 0u : runtime.FreeMemory(nil);
}

std::uint32_t RedBlackTreeCollection::ConstructWithArgs(
    const std::uint32_t first_source, const std::uint8_t third_value,
    const std::uint32_t second_source,
    IRedBlackTreeCollectionRuntime& runtime) const {
    runtime.WriteByte(address_, runtime.ReadByte(second_source));
    runtime.WriteByte(address_ + 8u, third_value);
    runtime.WriteByte(address_ + 1u, runtime.ReadByte(first_source));
    runtime.ConstructTree(address_);
    return address_;
}

std::uint32_t RedBlackTreeCollection::LeftmostNode(
    const std::uint32_t root_slot, IRedBlackTreeCollectionRuntime& runtime) {
    auto result = root_slot;
    auto node = runtime.ReadDword(root_slot);
    while (node != runtime.SharedNil()) {
        result = node;
        node = runtime.ReadDword(node);
    }
    return result;
}

std::uint32_t RedBlackTreeCollection::AdvanceIterator(
    std::uint32_t& node, IRedBlackTreeCollectionRuntime& runtime) {
    auto right = runtime.ReadDword(node + 8u);
    if (right != runtime.SharedNil()) {
        auto left = runtime.ReadDword(right);
        while (left != runtime.SharedNil()) {
            right = left;
            left = runtime.ReadDword(right);
        }
        node = right;
        return left;
    }

    auto parent = runtime.ReadDword(node + 4u);
    while (node == runtime.ReadDword(parent + 8u)) {
        node = parent;
        parent = runtime.ReadDword(parent + 4u);
    }
    if (runtime.ReadDword(node + 8u) != parent) node = parent;
    return parent;
}

std::uint32_t RedBlackTreeCollection::FreeSubtree(
    std::uint32_t root, IRedBlackTreeCollectionRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (root != nil) {
        (void)FreeSubtree(runtime.ReadDword(root + 8u), runtime);
        const auto left = runtime.ReadDword(root);
        (void)runtime.FreeMemory(root);
        root = left;
    }
    return nil;
}

std::uint32_t RedBlackTreeCollection::ConstructExtended(
    IRedBlackTreeConstructorRuntime& runtime) const {
    const auto candidate = runtime.AllocateMemory(0x14);
    runtime.WriteDword(candidate + 16u, 1);
    runtime.WriteDword(candidate + 4u, 0);
    if (runtime.SharedNil() == 0) {
        runtime.SetSharedNil(candidate);
        runtime.WriteDword(candidate, 0);
        runtime.WriteDword(candidate + 8u, 0);
    }
    runtime.SetSharedNilUsers(runtime.SharedNilUsers() + 1);
    if (candidate != runtime.SharedNil())
        (void)runtime.FreeMemory(candidate);

    const auto head = runtime.AllocateMemory(0x14);
    runtime.WriteDword(head + 4u, runtime.SharedNil());
    runtime.WriteDword(head + 16u, 0);
    runtime.WriteDword(address_ + 4u, head);
    runtime.WriteDword(address_ + 12u, 0);
    runtime.WriteDword(head, head);
    runtime.WriteDword(head + 8u, head);
    return head;
}

std::uint32_t& RedBlackTreeCollection::DeleteRange(
    std::uint32_t& output, std::uint32_t first, const std::uint32_t last,
    IRedBlackTreeCollectionRuntime& runtime) const {
    if (runtime.ReadDword(address_ + 12u) != 0u) {
        const auto header = runtime.ReadDword(address_ + 4u);
        if (first == runtime.ReadDword(header) && last == header) {
            auto node = runtime.ReadDword(header + 4u);
            while (node != runtime.SharedNil()) {
                runtime.FreeNodes(address_, runtime.ReadDword(node + 8u));
                const auto left = runtime.ReadDword(node);
                (void)runtime.FreeMemory(node);
                node = left;
            }
            runtime.WriteDword(header + 4u, runtime.SharedNil());
            runtime.WriteDword(address_ + 12u, 0u);
            runtime.WriteDword(header, header);
            runtime.WriteDword(header + 8u, header);
            output = runtime.ReadDword(header);
            return output;
        }
    }

    while (first != last) {
        const auto current = first;
        (void)AdvanceIterator(first, runtime);
        runtime.DeleteNode(address_, current);
    }
    output = first;
    return output;
}

std::uint32_t& RedBlackTreeCollection::ClearRange(
    std::uint32_t& output, const std::uint32_t first,
    const std::uint32_t last,
    IRedBlackTreeCollectionRuntime& runtime) const {
    return DeleteRange(output, first, last, runtime);
}

}  // namespace pal4
