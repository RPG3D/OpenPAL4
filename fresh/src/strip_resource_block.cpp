#include "pal4/strip_resource_block.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t StripResourceBlock::Construct(
    std::uint32_t block, IStripResourceBlockRuntime& runtime) {
    const auto stack_byte = runtime.StackByte();
    runtime.WriteByte(block + 1u, 1u);
    runtime.WriteByte(block + 2u, 1u);
    runtime.WriteByte(block, 0u);
    runtime.WriteByte(block + 4u, 0u);
    for (const auto offset : {56u, 60u, 64u, 100u, 104u})
        runtime.WriteWord(block + offset, 0u);
    runtime.WriteByte(block + 108u, stack_byte);
    const auto head = runtime.Allocate(0x24u);
    if (head == 0u) return 0u;
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 4u, head);
    runtime.WriteWord(block + 112u, head);
    runtime.WriteWord(block + 116u, 0u);
    runtime.WriteByte(block + 120u, stack_byte);
    for (const auto offset : {124u, 128u, 132u})
        runtime.WriteWord(block + offset, 0u);
    return block;
}

}  // namespace pal4
