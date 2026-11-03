#include "pal4/ui_red_black_tree_insert.h"

namespace pal4 {
namespace {

std::uint32_t Read(std::uint32_t address,
                   IUiRedBlackTreeInsertRuntime& runtime) {
    return runtime.ReadWord(address);
}

void RotateLeft(std::uint32_t tree, std::uint32_t node,
                IUiRedBlackTreeInsertRuntime& runtime) {
    (void)UiRedBlackTreeRotation::RotateLeft(tree, node, runtime);
}

void RotateRight(std::uint32_t tree, std::uint32_t node,
                 IUiRedBlackTreeInsertRuntime& runtime) {
    (void)UiRedBlackTreeRotation::RotateRight(tree, node, runtime);
}

void Repair(std::uint32_t tree, std::uint32_t node,
            IUiRedBlackTreeInsertRuntime& runtime) {
    const auto head = Read(tree + 4u, runtime);
    while (node != Read(head + 4u, runtime) &&
           Read(Read(node + 4u, runtime) + 32u, runtime) == 0u) {
        const auto parent = Read(node + 4u, runtime);
        const auto grandparent = Read(parent + 4u, runtime);
        if (parent == Read(grandparent, runtime)) {
            const auto uncle = Read(grandparent + 8u, runtime);
            if (Read(uncle + 32u, runtime) == 0u) {
                runtime.WriteWord(parent + 32u, 1u);
                runtime.WriteWord(uncle + 32u, 1u);
                runtime.WriteWord(grandparent + 32u, 0u);
                node = grandparent;
            } else {
                if (node == Read(parent + 8u, runtime)) {
                    node = parent;
                    RotateLeft(tree, node, runtime);
                }
                const auto current_parent = Read(node + 4u, runtime);
                const auto current_grandparent =
                    Read(current_parent + 4u, runtime);
                runtime.WriteWord(current_parent + 32u, 1u);
                runtime.WriteWord(current_grandparent + 32u, 0u);
                RotateRight(tree, current_grandparent, runtime);
            }
        } else {
            const auto uncle = Read(grandparent, runtime);
            if (Read(uncle + 32u, runtime) == 0u) {
                runtime.WriteWord(parent + 32u, 1u);
                runtime.WriteWord(uncle + 32u, 1u);
                runtime.WriteWord(grandparent + 32u, 0u);
                node = grandparent;
            } else {
                if (node == Read(parent, runtime)) {
                    node = parent;
                    RotateRight(tree, node, runtime);
                }
                const auto current_parent = Read(node + 4u, runtime);
                const auto current_grandparent =
                    Read(current_parent + 4u, runtime);
                runtime.WriteWord(current_parent + 32u, 1u);
                runtime.WriteWord(current_grandparent + 32u, 0u);
                RotateLeft(tree, current_grandparent, runtime);
            }
        }
    }
    runtime.WriteWord(Read(head + 4u, runtime) + 32u, 1u);
}

}  // namespace

std::uint32_t UiRedBlackTreeInsert::Insert(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t key, IUiRedBlackTreeInsertRuntime& runtime) {
    const auto node = runtime.AllocateMemory(0x24u);
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 32u, 0u);
    const auto nil = runtime.SharedNil();
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    runtime.CopyText(node + 12u, key);
    const auto head = Read(tree + 4u, runtime);
    runtime.WriteWord(tree + 12u, Read(tree + 12u, runtime) + 1u);
    if (parent == head || child != nil ||
        runtime.CompareText(key, parent + 12u) < 0) {
        runtime.WriteWord(parent, node);
        if (parent == head) {
            runtime.WriteWord(head + 4u, node);
            runtime.WriteWord(head + 8u, node);
        } else if (parent == Read(head, runtime)) {
            runtime.WriteWord(head, node);
        }
    } else {
        runtime.WriteWord(parent + 8u, node);
        if (parent == Read(head + 8u, runtime))
            runtime.WriteWord(head + 8u, node);
    }
    Repair(tree, node, runtime);
    runtime.WriteWord(output, node);
    return output;
}

}  // namespace pal4
