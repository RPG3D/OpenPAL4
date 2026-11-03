#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::ConstructEmbeddedTree(
    const std::uint32_t tree, const std::uint32_t first_source,
    const std::uint8_t flag, const std::uint32_t third_source,
    IUiFrameManagerTreeConstructRuntime& runtime) {
    const auto third_byte = runtime.ReadByte(third_source);
    runtime.WriteByte(tree, third_byte);
    const auto first_byte = runtime.ReadByte(first_source);
    runtime.WriteByte(tree + 8u, flag);
    runtime.WriteByte(tree + 1u, first_byte);
    runtime.ConstructRedBlackTree(tree);
    return tree;
}

}  // namespace pal4
