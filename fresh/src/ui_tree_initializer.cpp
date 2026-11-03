#include "pal4/ui_tree_initializer.h"

namespace pal4 {

std::uint32_t UiTreeInitializer::Initialize(
    const std::uint32_t tree, const std::uint32_t first_byte_source,
    const std::uint8_t flag, const std::uint32_t root_byte_source,
    IUiTreeInitializerRuntime& runtime) {
    runtime.WriteByte(tree, runtime.ReadByte(root_byte_source));
    const auto second = runtime.ReadByte(first_byte_source);
    runtime.WriteByte(tree + 8u, flag);
    runtime.WriteByte(tree + 1u, second);
    runtime.InitializeTree(tree);
    return tree;
}

}  // namespace pal4
