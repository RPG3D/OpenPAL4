#include "pal4/script_attribute_tree_lookup.h"

#include <algorithm>

namespace pal4 {

std::uint32_t ScriptAttributeTreeRuntimeStub::NilAddress() { return 0u; }
std::uint32_t ScriptAttributeTreeRuntimeStub::EmptyStringAddress() {
    return 0u;
}
std::uint32_t ScriptAttributeTreeRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
std::int32_t ScriptAttributeTreeRuntimeStub::CompareNodeKey(
    std::uint32_t, std::uint32_t) { return 0; }
std::int32_t ScriptAttributeTreeRuntimeStub::CompareBytes(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0; }

std::uint32_t& ScriptAttributeTreeLookup::Find(
    std::uint32_t tree, std::uint32_t& output,
    std::uint32_t query, IScriptAttributeTreeRuntime& runtime) {
    const auto header = runtime.ReadDword(tree + 4u);
    auto node = runtime.ReadDword(header + 4u);
    auto candidate = header;
    const auto nil = runtime.NilAddress();
    while (node != nil) {
        if (runtime.CompareNodeKey(node + 12u, query) >= 0) {
            candidate = node;
            node = runtime.ReadDword(node);
        } else {
            node = runtime.ReadDword(node + 8u);
        }
    }

    output = header;
    if (candidate == header) return output;
    const auto node_length = runtime.ReadDword(candidate + 20u);
    auto node_data = runtime.ReadDword(candidate + 16u);
    if (node_data == 0u) node_data = runtime.EmptyStringAddress();
    const auto query_length = runtime.ReadDword(query + 8u);
    const auto query_data = runtime.ReadDword(query + 4u);
    const auto comparison = runtime.CompareBytes(
        query_data, node_data, std::min(query_length, node_length));
    if (comparison >= 0 &&
        (comparison != 0 || query_length >= node_length))
        output = candidate;
    return output;
}

}  // namespace pal4
