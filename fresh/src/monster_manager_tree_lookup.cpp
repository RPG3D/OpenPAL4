#include "pal4/monster_manager_tree_lookup.h"

#include <algorithm>

namespace pal4 {

std::uint32_t MonsterManagerTreeLookup::Search(
    std::uint32_t tree, std::uint32_t input,
    IMonsterManagerTreeLookupRuntime& runtime) {
    const auto header = runtime.ReadDword(tree + 4u);
    auto candidate = header;
    auto node = runtime.ReadDword(header + 4u);
    while (node != runtime.SharedNil()) {
        if (runtime.CompareText(node + 0x0cu, input) >= 0) {
            candidate = node;
            node = runtime.ReadDword(node);
        } else {
            node = runtime.ReadDword(node + 8u);
        }
    }
    if (candidate == header) return header;

    auto node_data = runtime.ReadDword(candidate + 0x10u);
    if (node_data == 0u) node_data = runtime.EmptyTextData();
    const auto node_length = runtime.ReadDword(candidate + 0x14u);
    const auto input_data = runtime.ReadDword(input + 4u);
    const auto input_length = runtime.ReadDword(input + 8u);
    for (std::uint32_t i = 0u; i < std::min(input_length, node_length); ++i) {
        const auto a = runtime.ReadByte(input_data + i);
        const auto b = runtime.ReadByte(node_data + i);
        if (a < b) return header;
        if (a > b) return candidate;
    }
    return input_length < node_length ? header : candidate;
}

std::uint32_t MonsterManagerTreeLookup::LowerBoundAlternative(
    std::uint32_t tree, std::uint32_t input,
    IMonsterManagerTreeLookupRuntime& runtime) {
    const auto header = runtime.ReadDword(tree + 4u);
    auto candidate = header;
    auto node = runtime.ReadDword(header + 4u);
    while (node != runtime.SharedNil()) {
        if (runtime.CompareRange(node + 0x0cu, input) >= 0) {
            candidate = node;
            node = runtime.ReadDword(node);
        } else {
            node = runtime.ReadDword(node + 8u);
        }
    }
    return candidate;
}

}  // namespace pal4
