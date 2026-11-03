#include "pal4/cegui_map_operations.h"

namespace pal4 {

std::uint32_t CeguiMapOperations::PreviousCursor(
    std::uint32_t& cursor, const CeguiTreeGlobals& globals,
    ICeguiMapCursorRuntime& runtime) {
    const auto node = cursor;
    const auto parent = runtime.ReadDword(node + 4u);
    if (runtime.ReadDword(node + 0x14u) == 0u &&
        runtime.ReadDword(parent + 4u) == node) {
        cursor = runtime.ReadDword(node + 8u);
        return cursor;
    }

    auto predecessor = runtime.ReadDword(node);
    if (predecessor != globals.nil) {
        auto scan = runtime.ReadDword(predecessor + 8u);
        while (scan != globals.nil) {
            predecessor = scan;
            scan = runtime.ReadDword(predecessor + 8u);
        }
        cursor = predecessor;
        return scan;
    }

    auto scan = parent;
    while (cursor == runtime.ReadDword(scan)) {
        cursor = scan;
        scan = runtime.ReadDword(scan + 4u);
    }
    cursor = scan;
    return scan;
}

std::uint32_t CeguiMapOperations::NextCursor(
    std::uint32_t& cursor, const CeguiTreeGlobals& globals,
    ICeguiMapCursorRuntime& runtime) {
    auto successor = runtime.ReadDword(cursor + 8u);
    if (successor != globals.nil) {
        auto scan = runtime.ReadDword(successor);
        while (scan != globals.nil) {
            successor = scan;
            scan = runtime.ReadDword(successor);
        }
        cursor = successor;
        return scan;
    }

    auto scan = runtime.ReadDword(cursor + 4u);
    while (cursor == runtime.ReadDword(scan + 8u)) {
        cursor = scan;
        scan = runtime.ReadDword(scan + 4u);
    }
    if (runtime.ReadDword(cursor + 8u) != scan) cursor = scan;
    return scan;
}

}  // namespace pal4
