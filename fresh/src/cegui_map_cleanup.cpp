#include "pal4/cegui_map_operations.h"

namespace pal4 {

std::uint32_t CeguiMapOperations::CleanupSubtree(
    const std::uint32_t root, const CeguiTreeGlobals& globals,
    ICeguiMapCleanupRuntime& runtime) {
    auto node = root;
    while (node != globals.nil) {
        static_cast<void>(CleanupSubtree(
            runtime.ReadDword(node + 8u), globals, runtime));
        const auto left = runtime.ReadDword(node);
        runtime.FreeNode(node);
        node = left;
    }
    return globals.nil;
}

}  // namespace pal4
