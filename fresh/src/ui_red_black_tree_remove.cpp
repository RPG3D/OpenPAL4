#include "pal4/ui_red_black_tree_remove.h"

namespace pal4 {
namespace {

std::uint32_t Read(std::uint32_t address,
                   IUiRedBlackTreeRemoveRuntime& runtime) {
    return runtime.ReadWord(address);
}

std::uint32_t Next(std::uint32_t node,
                   IUiRedBlackTreeRemoveRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto right = Read(node + 8u, runtime);
    if (right != nil) {
        while (Read(right, runtime) != nil)
            right = Read(right, runtime);
        return right;
    }
    auto parent = Read(node + 4u, runtime);
    while (node == Read(parent + 8u, runtime)) {
        node = parent;
        parent = Read(parent + 4u, runtime);
    }
    return Read(node + 8u, runtime) == parent ? node : parent;
}

void RotateTowardLeft(std::uint32_t tree, std::uint32_t node,
                      IUiRedBlackTreeRemoveRuntime& runtime) {
    (void)UiRedBlackTreeRotation::RotateLeft(tree, node, runtime);
}

void RotateTowardRight(std::uint32_t tree, std::uint32_t node,
                       IUiRedBlackTreeRemoveRuntime& runtime) {
    (void)UiRedBlackTreeRotation::RotateRight(tree, node, runtime);
}

void Repair(std::uint32_t tree, std::uint32_t head, std::uint32_t child,
            std::uint32_t colour_offset,
            IUiRedBlackTreeRemoveRuntime& runtime) {
    while (child != Read(head + 4u, runtime) &&
           Read(child + colour_offset, runtime) == 1u) {
        const auto parent = Read(child + 4u, runtime);
        if (child == Read(parent, runtime)) {
            auto sibling = Read(parent + 8u, runtime);
            if (Read(sibling + colour_offset, runtime) == 0u) {
                runtime.WriteWord(sibling + colour_offset, 1u);
                runtime.WriteWord(parent + colour_offset, 0u);
                RotateTowardLeft(tree, parent, runtime);
                sibling = Read(Read(child + 4u, runtime) + 8u, runtime);
            }
            if (Read(Read(sibling, runtime) + colour_offset, runtime) == 1u &&
                Read(Read(sibling + 8u, runtime) + colour_offset,
                     runtime) == 1u) {
                runtime.WriteWord(sibling + colour_offset, 0u);
                child = Read(child + 4u, runtime);
            } else {
                if (Read(Read(sibling + 8u, runtime) + colour_offset,
                         runtime) == 1u) {
                    runtime.WriteWord(Read(sibling, runtime) + colour_offset,
                                      1u);
                    runtime.WriteWord(sibling + colour_offset, 0u);
                    RotateTowardRight(tree, sibling, runtime);
                    sibling = Read(Read(child + 4u, runtime) + 8u, runtime);
                }
                const auto current_parent = Read(child + 4u, runtime);
                runtime.WriteWord(sibling + colour_offset,
                                  Read(current_parent + colour_offset,
                                       runtime));
                runtime.WriteWord(current_parent + colour_offset, 1u);
                runtime.WriteWord(Read(sibling + 8u, runtime) + colour_offset,
                                  1u);
                RotateTowardLeft(tree, current_parent, runtime);
                break;
            }
        } else {
            auto sibling = Read(parent, runtime);
            if (Read(sibling + colour_offset, runtime) == 0u) {
                runtime.WriteWord(sibling + colour_offset, 1u);
                runtime.WriteWord(parent + colour_offset, 0u);
                RotateTowardRight(tree, parent, runtime);
                sibling = Read(Read(child + 4u, runtime), runtime);
            }
            if (Read(Read(sibling + 8u, runtime) + colour_offset,
                     runtime) == 1u &&
                Read(Read(sibling, runtime) + colour_offset, runtime) == 1u) {
                runtime.WriteWord(sibling + colour_offset, 0u);
                child = Read(child + 4u, runtime);
            } else {
                if (Read(Read(sibling, runtime) + colour_offset,
                         runtime) == 1u) {
                    runtime.WriteWord(Read(sibling + 8u, runtime) +
                                          colour_offset,
                                      1u);
                    runtime.WriteWord(sibling + colour_offset, 0u);
                    RotateTowardLeft(tree, sibling, runtime);
                    sibling = Read(Read(child + 4u, runtime), runtime);
                }
                const auto current_parent = Read(child + 4u, runtime);
                runtime.WriteWord(sibling + colour_offset,
                                  Read(current_parent + colour_offset,
                                       runtime));
                runtime.WriteWord(current_parent + colour_offset, 1u);
                runtime.WriteWord(Read(sibling, runtime) + colour_offset, 1u);
                RotateTowardRight(tree, current_parent, runtime);
                break;
            }
        }
    }
    runtime.WriteWord(child + colour_offset, 1u);
}

template <typename Cleanup>
std::uint32_t RemoveImpl(
    const std::uint32_t tree, const std::uint32_t node,
    const std::uint32_t colour_offset,
    IUiRedBlackTreeRemoveRuntime& runtime, Cleanup cleanup) {
    const auto nil = runtime.SharedNil();
    const auto head = Read(tree + 4u, runtime);
    const auto next = Next(node, runtime);
    auto removed = node;
    std::uint32_t child;
    if (Read(node, runtime) == nil) {
        child = Read(node + 8u, runtime);
    } else if (Read(node + 8u, runtime) == nil) {
        child = Read(node, runtime);
    } else {
        removed = Read(node + 8u, runtime);
        while (Read(removed, runtime) != nil)
            removed = Read(removed, runtime);
        child = Read(removed + 8u, runtime);
    }

    if (removed != node) {
        const auto left = Read(node, runtime);
        runtime.WriteWord(left + 4u, removed);
        runtime.WriteWord(removed, left);
        if (removed == Read(node + 8u, runtime)) {
            runtime.WriteWord(child + 4u, removed);
        } else {
            const auto old_parent = Read(removed + 4u, runtime);
            runtime.WriteWord(child + 4u, old_parent);
            runtime.WriteWord(old_parent, child);
            const auto right = Read(node + 8u, runtime);
            runtime.WriteWord(removed + 8u, right);
            runtime.WriteWord(right + 4u, removed);
        }
        const auto parent = Read(node + 4u, runtime);
        if (Read(head + 4u, runtime) == node)
            runtime.WriteWord(head + 4u, removed);
        else if (Read(parent, runtime) == node)
            runtime.WriteWord(parent, removed);
        else
            runtime.WriteWord(parent + 8u, removed);
        runtime.WriteWord(removed + 4u, parent);
        const auto colour = Read(removed + colour_offset, runtime);
        runtime.WriteWord(removed + colour_offset,
                          Read(node + colour_offset, runtime));
        runtime.WriteWord(node + colour_offset, colour);
        removed = node;
    } else {
        const auto parent = Read(node + 4u, runtime);
        runtime.WriteWord(child + 4u, parent);
        if (Read(head + 4u, runtime) == node)
            runtime.WriteWord(head + 4u, child);
        else if (Read(parent, runtime) == node)
            runtime.WriteWord(parent, child);
        else
            runtime.WriteWord(parent + 8u, child);
        if (Read(head, runtime) == node) {
            if (Read(node + 8u, runtime) == nil) {
                runtime.WriteWord(head, parent);
            } else {
                auto first = child;
                while (Read(first, runtime) != nil)
                    first = Read(first, runtime);
                runtime.WriteWord(head, first);
            }
        }
        if (Read(head + 8u, runtime) == node) {
            if (Read(node, runtime) == nil) {
                runtime.WriteWord(head + 8u, parent);
            } else {
                auto last = child;
                while (Read(last + 8u, runtime) != nil)
                    last = Read(last + 8u, runtime);
                runtime.WriteWord(head + 8u, last);
            }
        }
    }

    if (Read(removed + colour_offset, runtime) == 1u)
        Repair(tree, head, child, colour_offset, runtime);
    cleanup(removed);
    runtime.FreeMemory(removed);
    runtime.WriteWord(tree + 12u, Read(tree + 12u, runtime) - 1u);
    return next;
}

void ReleaseText(std::uint32_t text_field,
                 IUiRedBlackTreeRemoveRuntime& runtime) {
    const auto text = Read(text_field, runtime);
    if (text != 0u) {
        const auto count = runtime.ReadByte(text - 1u);
        if (count == 0u || count == 0xffu)
            runtime.FreeMemory(text - 1u);
        else
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    runtime.WriteWord(text_field, 0u);
    runtime.WriteWord(text_field + 4u, 0u);
    runtime.WriteWord(text_field + 8u, 0u);
}

}  // namespace

std::uint32_t UiRedBlackTreeRemove::Remove(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t node, IUiRedBlackTreeRemoveRuntime& runtime) {
    runtime.WriteWord(output, RemoveImpl(tree, node, 32u, runtime,
                                        [&](std::uint32_t removed) {
                                            ReleaseText(removed + 16u, runtime);
                                        }));
    return output;
}

std::uint32_t UiRedBlackTreeRemove::RemoveEffectNode(
    const std::uint32_t tree, const std::uint32_t node,
    IUiRedBlackTreeRemoveRuntime& runtime) {
    return RemoveImpl(tree, node, 32u, runtime,
                      [](std::uint32_t) {});
}

std::uint32_t UiRedBlackTreeRemove::RemoveTextureNode(
    const std::uint32_t tree, const std::uint32_t node,
    IUiRedBlackTreeRemoveRuntime& runtime) {
    return RemoveImpl(tree, node, 36u, runtime,
                      [&](std::uint32_t removed) {
                          ReleaseText(removed + 16u, runtime);
                      });
}

std::uint32_t UiRedBlackTreeRemove::RemoveCegui(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t node,
    IUiCeguiRedBlackTreeRemoveRuntime& runtime) {
    runtime.WriteWord(output, RemoveImpl(tree, node, 180u, runtime,
                                        [&](std::uint32_t removed) {
                                            ReleaseText(removed + 168u, runtime);
                                            runtime.DestroyCeguiString(removed + 12u);
                                        }));
    return output;
}

std::uint32_t UiRedBlackTreeRemove::RemoveFactory(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t node, IUiRedBlackTreeRemoveRuntime& runtime) {
    runtime.WriteWord(output, RemoveImpl(tree, node, 20u, runtime,
                                        [](std::uint32_t) {}));
    return output;
}

}  // namespace pal4
