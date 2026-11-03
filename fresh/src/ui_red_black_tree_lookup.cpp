#include "pal4/ui_red_black_tree_lookup.h"

namespace pal4 {

std::uint32_t UiRedBlackTreeLookup::LowerBound(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t query,
    IUiRedBlackTreeLookupRuntime& runtime) {
    auto candidate = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(candidate + 4u);
    const auto nil = runtime.SharedNil();
    while (node != nil) {
        auto text = runtime.ReadWord(query + 4u);
        const auto length = runtime.ReadWord(query + 8u);
        if (text == 0u) text = runtime.EmptyText();
        const auto comparison = runtime.CompareRange(
            node + 12u, runtime.ReadWord(node + 20u), text, length);
        if (comparison >= 0) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    runtime.WriteWord(output, candidate);
    return output;
}

std::uint32_t UiRedBlackTreeLookup::LeftmostFromSlot(
    const std::uint32_t slot,
    IUiRedBlackTreeLookupRuntime& runtime) {
    auto result = slot;
    auto node = runtime.ReadWord(slot);
    while (node != runtime.SharedNil()) {
        result = node;
        node = runtime.ReadWord(node);
    }
    return result;
}

}  // namespace pal4
