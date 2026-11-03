#include "pal4/actor_manager_player_text_property.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t ActorManagerPlayerTextProperty::Assign(
    const std::uint32_t actor, const std::int32_t index,
    const std::uint32_t source,
    IActorManagerPlayerTextRuntime& runtime) {
    if (index < 0 || index >= 4) {
        const auto guard = runtime.ReadByte(kGuard);
        if ((guard & 1u) == 0u) {
            runtime.WriteByte(kGuard, static_cast<std::uint8_t>(guard | 1u));
            runtime.WriteByte(kFallback, static_cast<std::uint8_t>(index));
            runtime.WriteWord(kFallback + 4u, 0u);
            runtime.WriteWord(kFallback + 8u, 0u);
            runtime.WriteWord(kFallback + 12u, 0u);
            if (runtime.AllocateText(kFallback, 5u, 1u)) {
                const auto buffer = runtime.ReadWord(kFallback + 4u);
                runtime.WriteLiteral(buffer, "ERROR");
                runtime.WriteWord(kFallback + 8u, 5u);
                runtime.WriteByte(buffer + 5u, 0u);
            }
            runtime.RegisterCleanup(kCleanupFunction);
        }
        return kFallback;
    }

    const auto destination = actor + 1112u +
                             static_cast<std::uint32_t>(index) * 16u;
    const auto source_size = runtime.ReadWord(source + 8u);
    if (destination == source) {
        if (runtime.ReadWord(destination + 8u) < source_size)
            throw std::out_of_range("actor text self assignment");
        runtime.DetachAndReset(destination);
        const auto current_size = runtime.ReadWord(destination + 8u);
        const auto difference = current_size - source_size;
        if (difference != 0u) {
            const auto buffer = runtime.ReadWord(destination + 4u);
            runtime.CopyBytes(buffer + source_size,
                              buffer + source_size + difference, 0u);
            if (runtime.AllocateText(destination, source_size, 0u))
                runtime.SetTextSize(destination, source_size);
        }
        runtime.DetachAndReset(destination);
        return destination;
    }

    const auto source_buffer = runtime.ReadWord(source + 4u);
    const auto text = source_buffer ? source_buffer : kEmptyText;
    if (source_size && runtime.ReadByte(text - 1u) < 0xfeu) {
        runtime.SetErrorFlag(destination, 1u);
        runtime.WriteWord(destination + 4u, text);
        runtime.WriteWord(destination + 8u, source_size);
        runtime.WriteWord(destination + 12u,
                          runtime.ReadWord(source + 12u));
        runtime.WriteByte(text - 1u,
                          static_cast<std::uint8_t>(
                              runtime.ReadByte(text - 1u) + 1u));
        return destination;
    }

    if (runtime.AllocateText(destination, source_size, 1u)) {
        const auto buffer = runtime.ReadWord(destination + 4u);
        runtime.CopyBytes(buffer, text, source_size);
        runtime.SetTextSize(destination, source_size);
    }
    return destination;
}

void ActorManagerPlayerTextProperty::Cleanup(
    IActorManagerPlayerTextRuntime& runtime) {
    const auto buffer = runtime.ReadWord(kFallback + 4u);
    if (buffer) {
        const auto references = runtime.ReadByte(buffer - 1u);
        if (references == 0u || references == 0xffu)
            runtime.Free(buffer - 1u);
        else
            runtime.WriteByte(buffer - 1u,
                              static_cast<std::uint8_t>(references - 1u));
    }
    runtime.WriteWord(kFallback + 4u, 0u);
    runtime.WriteWord(kFallback + 8u, 0u);
    runtime.WriteWord(kFallback + 12u, 0u);
}

}  // namespace pal4
