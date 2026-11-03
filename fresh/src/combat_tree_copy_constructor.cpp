#include "pal4/combat_tree_copy_constructor.h"

#include <new>

namespace pal4 {
namespace {

void CopyNodes(std::uint32_t destination, std::uint32_t source,
               ICombatTreeCopyRuntime& runtime) {
    const auto nil = runtime.Nil();
    const auto head = runtime.ReadWord(destination + 4u);
    const auto source_head = runtime.ReadWord(source + 4u);
    const auto source_root = runtime.ReadWord(source_head + 4u);
    const auto root = CombatResultTree::CopyTree(source_root, head, runtime);
    runtime.WriteWord(head + 4u, root);
    runtime.WriteWord(destination + 12u,
                      runtime.ReadWord(source + 12u));
    if (root == nil) {
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
    } else {
        auto minimum = root;
        while (runtime.ReadWord(minimum) != nil)
            minimum = runtime.ReadWord(minimum);
        runtime.WriteWord(head, minimum);
        auto maximum = root;
        while (runtime.ReadWord(maximum + 8u) != nil)
            maximum = runtime.ReadWord(maximum + 8u);
        runtime.WriteWord(head + 8u, maximum);
    }
}

}  // namespace

std::uint32_t CombatTreeCopyConstructor::Copy(
    std::uint32_t destination, std::uint32_t source,
    ICombatTreeCopyRuntime& runtime) {
    bool head_ready = false;
    try {
        runtime.WriteByte(destination, runtime.ReadByte(source));
        runtime.WriteByte(destination + 1u, runtime.ReadByte(source + 1u));
        runtime.WriteByte(destination + 8u, runtime.ReadByte(source + 8u));
        (void)InitializeTree(destination, runtime);
        head_ready = true;
        CopyNodes(destination, source, runtime);
        return destination;
    } catch (...) {
        if (head_ready) runtime.DestroyTree(destination);
        throw;
    }
}

std::uint32_t CombatTreeCopyConstructor::Assign(
    std::uint32_t destination, std::uint32_t source,
    ICombatTreeCopyRuntime& runtime) {
    runtime.WriteByte(destination, runtime.ReadByte(source));
    runtime.WriteByte(destination + 1u, runtime.ReadByte(source + 1u));
    runtime.WriteByte(destination + 8u, runtime.ReadByte(source + 8u));
    (void)InitializeTree(destination, runtime);
    try {
        (void)CopyAssignedNodes(destination, source, runtime);
    } catch (...) {
        runtime.DestroyTree(destination);
        throw;
    }
    return destination;
}

std::uint32_t CombatTreeCopyConstructor::CopyAssignedNodes(
    std::uint32_t destination, std::uint32_t source,
    ICombatTreeCopyRuntime& runtime) {
    CopyNodes(destination, source, runtime);
    return runtime.Nil();
}

std::uint32_t CombatTreeCopyConstructor::InitializeTree(
    std::uint32_t tree, ICombatTreeCopyRuntime& runtime) {
    const auto candidate = runtime.Allocate(24u);
    if (candidate == 0u) throw std::bad_alloc();
    runtime.WriteWord(candidate + 20u, 1u);
    runtime.WriteWord(candidate + 4u, 0u);
    auto nil = runtime.ReadWord(kSharedNilAddress);
    if (nil == 0u) {
        nil = candidate;
        runtime.WriteWord(kSharedNilAddress, nil);
        runtime.WriteWord(nil, 0u);
        runtime.WriteWord(nil + 8u, 0u);
    }
    runtime.WriteWord(kSharedRefCountAddress,
                      runtime.ReadWord(kSharedRefCountAddress) + 1u);
    if (candidate != nil) runtime.Free(candidate);

    std::uint32_t head = 0u;
    try {
        head = runtime.Allocate(24u);
        if (head == 0u) throw std::bad_alloc();
        runtime.WriteWord(head + 4u, nil);
        runtime.WriteWord(head + 20u, 0u);
        runtime.WriteWord(tree + 4u, head);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
    } catch (...) {
        if (head != 0u) runtime.Free(head);
        const auto remaining = runtime.ReadWord(kSharedRefCountAddress) - 1u;
        runtime.WriteWord(kSharedRefCountAddress, remaining);
        if (remaining == 0u) {
            runtime.WriteWord(kSharedNilAddress, 0u);
            runtime.Free(nil);
        }
        throw;
    }
    return head;
}

}  // namespace pal4
