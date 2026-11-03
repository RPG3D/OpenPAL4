#include "pal4/actor_manager_player_text_lookup.h"

namespace pal4 {

std::uint32_t ActorManagerPlayerTextLookup::Record(
    const std::uint32_t base, const std::int32_t index,
    IActorManagerPlayerTextRuntime& runtime) {
    if (index >= 0 && index < 4)
        return base + static_cast<std::uint32_t>(index) * 16u;

    const auto guard = runtime.ReadByte(kGuard);
    if ((guard & 1u) == 0u) {
        runtime.WriteByte(kGuard, static_cast<std::uint8_t>(guard | 1u));
        runtime.WriteByte(kFallback, static_cast<std::uint8_t>(index));
        runtime.WriteWord(kFallback + 4u, 0u);
        runtime.WriteWord(kFallback + 8u, 0u);
        runtime.WriteWord(kFallback + 12u, 0u);
        runtime.SetErrorFlag(kFallback, 1u);
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

void ActorManagerPlayerTextLookup::Cleanup(
    IActorManagerPlayerTextRuntime& runtime) {
    const auto buffer = runtime.ReadWord(kFallback + 4u);
    if (buffer != 0u) {
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
