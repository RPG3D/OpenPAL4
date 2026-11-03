#include "pal4/combat_child_text_assignment.h"

#include <stdexcept>

namespace pal4 {
namespace {

std::uint32_t TextOrEmpty(ICombatChildTextRuntime& runtime,
                          const std::uint32_t header) {
    const auto data = runtime.ReadWord(header + 4u);
    return data == 0u ? 0x841964u : data;
}

}  // namespace

std::uint8_t CombatChildTextAssignment::Assign(
    const std::uint32_t destination,
    const std::uint32_t source_header,
    ICombatChildTextRuntime& runtime) {
    if (runtime.TryAssignHostText(destination, source_header)) return 1u;
    const auto length = runtime.ReadWord(source_header + 8u);
    if (destination == source_header) {
        if (runtime.ReadWord(destination + 8u) < length)
            throw std::out_of_range("legacy text alias length");
        runtime.DetachText(destination);
        const auto remaining = runtime.ReadWord(destination + 8u) - length;
        std::uint32_t erase_count = 0xffffffffu;
        if (remaining != 0xffffffffu) erase_count = remaining;
        if (remaining == 0xffffffffu || erase_count != 0u) {
            const auto data = runtime.ReadWord(destination + 4u);
            runtime.MoveTextBytes(data + length,
                                  data + length + erase_count,
                                  remaining - erase_count);
            const auto new_length =
                runtime.ReadWord(destination + 8u) - erase_count;
            if (runtime.AllocateText(destination, new_length, 0u))
                runtime.SetTextSize(destination, new_length);
        }
        return runtime.EraseText(destination, 0u, 0u);
    }

    if (length != 0u) {
        const auto data = TextOrEmpty(runtime, source_header);
        const auto references = runtime.ReadByte(data - 1u);
        if (references < 0xfeu) {
            runtime.SetTextErrorFlag(destination, 1u);
            const auto shared_data = TextOrEmpty(runtime, source_header);
            runtime.WriteWord(destination + 4u, shared_data);
            runtime.WriteWord(destination + 8u,
                              runtime.ReadWord(source_header + 8u));
            runtime.WriteWord(destination + 12u,
                              runtime.ReadWord(source_header + 12u));
            runtime.WriteByte(shared_data - 1u,
                              static_cast<std::uint8_t>(references + 1u));
            return static_cast<std::uint8_t>(shared_data);
        }
    }
    if (!runtime.AllocateText(destination, length, 1u)) return 0u;
    runtime.CopyText(destination, TextOrEmpty(runtime, source_header), length);
    runtime.WriteWord(destination + 8u, length);
    runtime.WriteByte(runtime.ReadWord(destination + 4u) + length, 0u);
    return static_cast<std::uint8_t>(length);
}

}  // namespace pal4
