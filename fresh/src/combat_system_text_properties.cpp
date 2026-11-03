#include "pal4/combat_system_text_properties.h"

namespace pal4 {

std::uint8_t CombatSystemTextProperties::AssignFirst(
    const std::uint32_t object, const std::uint32_t source,
    ICombatSystemTextPropertyRuntime& runtime) {
    return AssignAt(object + 452u, source, runtime);
}

std::uint8_t CombatSystemTextProperties::AssignSecond(
    const std::uint32_t object, const std::uint32_t source,
    ICombatSystemTextPropertyRuntime& runtime) {
    return AssignAt(object + 468u, source, runtime);
}

std::uint8_t CombatSystemTextProperties::AssignThird(
    const std::uint32_t object, const std::uint32_t source,
    ICombatSystemTextPropertyRuntime& runtime) {
    return AssignAt(object + 484u, source, runtime);
}

std::uint8_t CombatSystemTextProperties::AssignAt(
    const std::uint32_t destination, const std::uint32_t source,
    ICombatSystemTextPropertyRuntime& runtime) {
    const auto length = runtime.ReadWord(source + 8u);
    if (destination == source) {
        if (runtime.ReadWord(destination + 8u) < length)
            runtime.ThrowRange();
        static_cast<void>(runtime.DetachAndReset(destination));
        const auto remaining = runtime.ReadWord(destination + 8u) - length;
        if (remaining != 0u) {
            const auto removed = remaining == 0xffffffffu ?
                0xffffffffu : remaining;
            const auto data = runtime.ReadWord(destination + 4u);
            runtime.CopyBytes(data + length,
                              data + length + removed,
                              remaining - removed);
            const auto new_length = runtime.ReadWord(destination + 8u) -
                                    removed;
            if (runtime.AllocateText(destination, new_length, 0u))
                runtime.SetTextSize(destination, new_length);
        }
        return runtime.DetachAndReset(destination);
    }

    const auto source_data = runtime.ReadWord(source + 4u) == 0u ?
        kEmptyText : runtime.ReadWord(source + 4u);
    if (length != 0u && runtime.ReadByte(source_data - 1u) < 0xfeu) {
        const auto old_data = runtime.ReadWord(destination + 4u);
        if (old_data != 0u) {
            const auto old_header = old_data - 1u;
            const auto references = runtime.ReadByte(old_header);
            if (references == 0u || references == 0xffu)
                runtime.Free(old_header);
            else
                runtime.WriteByte(old_header,
                                  static_cast<std::uint8_t>(references - 1u));
        }
        runtime.WriteWord(destination + 4u, 0u);
        runtime.WriteWord(destination + 8u, 0u);
        runtime.WriteWord(destination + 12u, 0u);
        runtime.WriteWord(destination + 4u, source_data);
        runtime.WriteWord(destination + 8u, length);
        runtime.WriteWord(destination + 12u,
                          runtime.ReadWord(source + 12u));
        runtime.WriteByte(source_data - 1u,
                          static_cast<std::uint8_t>(
                              runtime.ReadByte(source_data - 1u) + 1u));
        return static_cast<std::uint8_t>(source_data);
    }

    if (!runtime.AllocateText(destination, length, 1u)) return 0u;
    runtime.CopyBytes(runtime.ReadWord(destination + 4u), source_data,
                      length);
    runtime.WriteWord(destination + 8u, length);
    runtime.WriteByte(runtime.ReadWord(destination + 4u) + length, 0u);
    return static_cast<std::uint8_t>(length);
}

}  // namespace pal4
