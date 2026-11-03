#include "pal4/ui_red_black_tree_iterator.h"

namespace pal4 {
namespace {

std::uint32_t PreviousWithColourOffset(
    const std::uint32_t cursor, const std::uint32_t colour_offset,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    auto node = runtime.ReadWord(cursor);
    const auto parent = runtime.ReadWord(node + 4u);
    if (runtime.ReadWord(node + colour_offset) == 0u &&
        runtime.ReadWord(parent + 4u) == node) {
        const auto last = runtime.ReadWord(node + 8u);
        runtime.WriteWord(cursor, last);
        return last;
    }

    auto left = runtime.ReadWord(node);
    const auto nil = runtime.SharedNil();
    if (left != nil) {
        auto right = runtime.ReadWord(left + 8u);
        while (right != nil) {
            left = right;
            right = runtime.ReadWord(left + 8u);
        }
        runtime.WriteWord(cursor, left);
        return right;
    }

    auto ancestor = parent;
    while (node == runtime.ReadWord(ancestor)) {
        runtime.WriteWord(cursor, ancestor);
        ancestor = runtime.ReadWord(ancestor + 4u);
        node = runtime.ReadWord(cursor);
    }
    runtime.WriteWord(cursor, ancestor);
    return ancestor;
}

}  // namespace

std::uint32_t UiRedBlackTreeIterator::Next(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    auto current = runtime.ReadWord(cursor);
    auto right = runtime.ReadWord(current + 8u);
    const auto nil = runtime.SharedNil();
    if (right != nil) {
        auto left = runtime.ReadWord(right);
        while (left != nil) {
            right = left;
            left = runtime.ReadWord(right);
        }
        runtime.WriteWord(cursor, right);
        return left;
    }

    auto parent = runtime.ReadWord(current + 4u);
    while (current == runtime.ReadWord(parent + 8u)) {
        runtime.WriteWord(cursor, parent);
        parent = runtime.ReadWord(parent + 4u);
        current = runtime.ReadWord(cursor);
    }
    current = runtime.ReadWord(cursor);
    if (runtime.ReadWord(current + 8u) != parent)
        runtime.WriteWord(cursor, parent);
    return parent;
}

std::uint32_t UiRedBlackTreeIterator::AdvanceAndReturnPrevious(
    const std::uint32_t cursor, const std::uint32_t output,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    const auto previous = runtime.ReadWord(cursor);
    auto right = runtime.ReadWord(previous + 8u);
    const auto nil = runtime.SharedNil();
    if (right != nil) {
        while (runtime.ReadWord(right) != nil)
            right = runtime.ReadWord(right);
        runtime.WriteWord(cursor, right);
    } else {
        auto parent = runtime.ReadWord(previous + 4u);
        while (runtime.ReadWord(cursor) == runtime.ReadWord(parent + 8u)) {
            runtime.WriteWord(cursor, parent);
            parent = runtime.ReadWord(parent + 4u);
        }
        if (runtime.ReadWord(runtime.ReadWord(cursor) + 8u) != parent)
            runtime.WriteWord(cursor, parent);
    }
    runtime.WriteWord(output, previous);
    return output;
}

std::uint32_t UiRedBlackTreeIterator::Previous(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return PreviousWithColourOffset(cursor, 32u, runtime);
}

std::uint32_t UiRedBlackTreeIterator::PreviousFactory(
    const std::uint32_t cursor,
    IUiRedBlackTreeIteratorRuntime& runtime) {
    return PreviousWithColourOffset(cursor, 20u, runtime);
}

}  // namespace pal4
