#include "pal4/ui_frame_manager_node_allocator.h"

namespace pal4 {

std::uint32_t UiFrameManagerNodeAllocator::AllocateAndInitialize(
    const std::uint32_t first, const std::uint32_t second,
    IUiFrameManagerNodeAllocatorRuntime& runtime) {
    const auto node = runtime.Allocate(0x28u);
    runtime.WriteWord(node, first != 0u ? first : node);
    runtime.WriteWord(node + 4u, second != 0u ? second : node);
    return node;
}

}  // namespace pal4
