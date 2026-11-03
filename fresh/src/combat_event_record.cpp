#include "pal4/combat_event_record.h"

namespace pal4 {
namespace {

void CopyBody(std::uint32_t destination, std::uint32_t source,
              std::uint32_t metadata, ICombatEventRecordRuntime& runtime) {
    const auto flag = runtime.ReadByte(source);
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteByte(destination, flag);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    const auto length = runtime.ReadWord(source + 8u);

    if (destination == source) {
        runtime.DetachAndReset(destination);
        const auto size = runtime.ReadWord(destination + 8u);
        if (size != 0u) {
            const auto offset = size == 0xffffffffu ? 0xffffffffu : size;
            const auto text = runtime.ReadWord(destination + 4u);
            runtime.CopyBytes(text, text + offset, size - offset);
            const auto new_size = size - offset;
            if (runtime.AllocateText(destination, new_size, false))
                runtime.SetTextSize(destination, new_size);
        }
        runtime.DetachAndReset(destination);
    } else {
        auto source_text = 0u;
        if (length != 0u) {
            source_text = runtime.ReadWord(source + 4u);
            if (source_text == 0u) source_text = runtime.EmptyTextAddress();
        }
        if (length != 0u && runtime.ReadByte(source_text - 1u) < 0xfeu) {
            runtime.SetErrorFlag(destination, 1u);
            runtime.WriteWord(destination + 4u, source_text);
            runtime.WriteWord(destination + 8u, length);
            runtime.WriteWord(destination + 12u,
                              runtime.ReadWord(source + 12u));
            runtime.WriteByte(source_text - 1u,
                              runtime.ReadByte(source_text - 1u) + 1u);
        } else if (runtime.AllocateText(destination, length, true)) {
            // AllocateText installs the destination pointer but leaves its
            // logical length unset until bytes and the terminator are written.
            const auto destination_text = runtime.ReadWord(destination + 4u);
            if (source_text == 0u) {
                source_text = runtime.ReadWord(source + 4u);
                if (source_text == 0u)
                    source_text = runtime.EmptyTextAddress();
            }
            runtime.CopyBytes(destination_text, source_text, length);
            runtime.WriteWord(destination + 8u, length);
            runtime.WriteByte(destination_text + length, 0u);
        }
    }
    runtime.WriteWord(destination + 16u, runtime.ReadWord(metadata));
}

}  // namespace

std::uint32_t CombatEventRecord::CopyData(
    std::uint32_t destination, std::uint32_t source,
    std::uint32_t metadata, ICombatEventRecordRuntime& runtime) {
    CopyBody(destination, source, metadata, runtime);
    return destination;
}

void CombatEventRecord::CopyExtended(
    std::uint32_t destination, std::uint32_t source,
    ICombatEventRecordRuntime& runtime) {
    if (destination != 0u)
        CopyBody(destination, source, source + 16u, runtime);
}

}  // namespace pal4
