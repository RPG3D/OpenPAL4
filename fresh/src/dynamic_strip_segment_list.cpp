#include "pal4/dynamic_strip_object_update.h"

namespace pal4 {

std::uint32_t DynamicStripSegmentList::Append(
    std::uint32_t sentinel, std::uint32_t& count,
    const DynamicStripSegment& segment,
    IDynamicStripSegmentListRuntime& runtime) {
    const auto previous = runtime.ReadWord(sentinel + 4u);
    const auto node = runtime.Allocate(36u);
    runtime.WriteWord(node, sentinel);
    const auto predecessor = previous != 0u ? previous : node;
    runtime.WriteWord(node + 4u, predecessor);
    runtime.WriteWord(sentinel + 4u, node);
    runtime.WriteWord(predecessor, node);
    runtime.CopySegment(node + 8u, segment);
    ++count;
    return node;
}

}  // namespace pal4
