#include "pal4/cegui_map_operations.h"

namespace pal4 {
namespace {

struct InsertionPlace {
    std::uint32_t header;
    std::uint32_t child;
    std::uint32_t parent;
    bool left;
};

InsertionPlace FindInsertionPlace(
    const std::uint32_t map, const std::uint32_t key,
    const CeguiTreeGlobals& globals, ICeguiMapRuntime& runtime) {
    const auto header = runtime.ReadDword(map + 4u);
    auto child = runtime.ReadDword(header + 4u);
    auto parent = header;
    bool left = true;
    while (child != globals.nil) {
        parent = child;
        left = key < runtime.ReadDword(child + 0x0cu);
        child = runtime.ReadDword(child + (left ? 0u : 8u));
    }
    return {header, child, parent, left};
}

}  // namespace

std::uint32_t CeguiMapOperations::ValueSlot(
    const std::uint32_t map, const std::uint32_t key,
    const CeguiTreeGlobals& globals, ICeguiMapRuntime& runtime) {
    const auto place = FindInsertionPlace(map, key, globals, runtime);
    const CeguiKeyMapping mapping{key, 0u};
    if (runtime.ReadByte(map + 8u) != 0u) {
        return runtime.InsertNode(map, place.child, place.parent,
                                  mapping) + 0x10u;
    }
    auto comparison_node = place.parent;
    if (place.left) {
        if (comparison_node == runtime.ReadDword(place.header)) {
            return runtime.InsertNode(map, place.child, place.parent,
                                      mapping) + 0x10u;
        }
        comparison_node = runtime.Predecessor(comparison_node);
    }
    if (runtime.ReadDword(comparison_node + 0x0cu) < key) {
        return runtime.InsertNode(map, place.child, place.parent,
                                  mapping) + 0x10u;
    }
    return comparison_node + 0x10u;
}

CeguiMapInsertionResult CeguiMapOperations::Insert(
    const std::uint32_t map, const CeguiKeyMapping mapping,
    const CeguiTreeGlobals& globals, ICeguiMapRuntime& runtime) {
    const auto place = FindInsertionPlace(
        map, mapping.windows_code, globals, runtime);
    if (runtime.ReadByte(map + 8u) != 0u) {
        return {runtime.InsertNode(map, place.child, place.parent,
                                   mapping), true};
    }
    auto comparison_node = place.parent;
    if (place.left) {
        if (comparison_node == runtime.ReadDword(place.header)) {
            return {runtime.InsertNode(map, place.child, place.parent,
                                       mapping), true};
        }
        comparison_node = runtime.Predecessor(comparison_node);
    }
    if (runtime.ReadDword(comparison_node + 0x0cu) <
        mapping.windows_code) {
        return {runtime.InsertNode(map, place.child, place.parent,
                                   mapping), true};
    }
    return {comparison_node, false};
}

std::uint32_t CeguiMapOperations::LowerBound(
    const std::uint32_t map, const std::uint32_t key,
    const CeguiTreeGlobals& globals, ICeguiMapRuntime& runtime) {
    auto candidate = runtime.ReadDword(map + 4u);
    auto node = runtime.ReadDword(candidate + 4u);
    while (node != globals.nil) {
        if (runtime.ReadDword(node + 0x0cu) >= key) {
            candidate = node;
            node = runtime.ReadDword(node);
        } else {
            node = runtime.ReadDword(node + 8u);
        }
    }
    return candidate;
}

}  // namespace pal4
