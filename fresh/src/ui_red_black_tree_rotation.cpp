#include "pal4/ui_red_black_tree_rotation.h"

namespace pal4 {

std::uint32_t UiRedBlackTreeRotation::RotateLeft(
    const std::uint32_t tree, const std::uint32_t node,
    IUiRedBlackTreeLinkRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node + 8u);
    const auto middle = runtime.ReadWord(pivot);
    runtime.WriteWord(node + 8u, middle);
    if (middle != runtime.SharedNil())
        runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent))
        runtime.WriteWord(parent, pivot);
    else
        runtime.WriteWord(parent + 8u, pivot);
    runtime.WriteWord(pivot, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

std::uint32_t UiRedBlackTreeRotation::RotateRight(
    const std::uint32_t tree, const std::uint32_t node,
    IUiRedBlackTreeLinkRuntime& runtime) {
    const auto pivot = runtime.ReadWord(node);
    const auto middle = runtime.ReadWord(pivot + 8u);
    runtime.WriteWord(node, middle);
    if (middle != runtime.SharedNil())
        runtime.WriteWord(middle + 4u, node);
    const auto parent = runtime.ReadWord(node + 4u);
    runtime.WriteWord(pivot + 4u, parent);
    const auto head = runtime.ReadWord(tree + 4u);
    if (node == runtime.ReadWord(head + 4u))
        runtime.WriteWord(head + 4u, pivot);
    else if (node == runtime.ReadWord(parent + 8u))
        runtime.WriteWord(parent + 8u, pivot);
    else
        runtime.WriteWord(parent, pivot);
    runtime.WriteWord(pivot + 8u, node);
    runtime.WriteWord(node + 4u, pivot);
    return pivot;
}

}  // namespace pal4
