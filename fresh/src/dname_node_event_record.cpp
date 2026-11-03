#include "pal4/dname_node_event_record.h"

#include <stdexcept>
#include <new>

namespace pal4 {

std::uint32_t DNameNode::Construct(
    std::uint32_t node, IDNameNodeRuntime& runtime) {
    runtime.WriteWord(node, 0xffffffffu);
    runtime.WriteWord(node + 4u, 0u);
    return node;
}

std::uint32_t DNameNodeEventRecord::SetString(
    const std::uint32_t record, const std::uint32_t first,
    const std::uint32_t second, const std::uint32_t text,
    IDNameNodeEventRuntime& runtime) {
    (void)DNameNode::Construct(record, runtime);
    runtime.WriteByte(record + 8u, static_cast<std::uint8_t>(text));
    runtime.WriteWord(record + 12u, 0u);
    runtime.WriteWord(record + 16u, 0u);
    runtime.WriteWord(record + 20u, 0u);
    runtime.WriteWord(record, first);
    runtime.WriteWord(record + 4u, second);

    const auto length = runtime.StringLength(text);
    if (length > 0xfffffffdu)
        throw std::length_error("DNameNode event text exceeds native limit");

    runtime.SetErrorFlag(record + 8u, 1u);
    if (length != 0u) {
        runtime.ReallocateString(record + 8u, length);
        const auto destination = runtime.ReadWord(record + 12u);
        if (destination == 0u)
            throw std::bad_alloc();
        runtime.CopyBytes(destination, text, length);
        runtime.WriteWord(record + 16u, length);
        runtime.WriteByte(destination + length, 0u);
    }
    return record;
}

}  // namespace pal4
