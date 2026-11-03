#include "pal4/actor_manager_combat_actor.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kEmptyText = 0x841964u;

void CopyTextRecord(const std::uint32_t destination,
                    const std::uint32_t source,
                    IActorManagerCombatCopyRuntime& runtime) {
    const auto length = runtime.ReadWord(source + 8u);
    if (destination == source) {
        runtime.EraseText(destination, length, 0xffffffffu);
        runtime.EraseText(destination, 0u, 0u);
        return;
    }

    if (length != 0u) {
        auto text = runtime.ReadWord(source + 4u);
        if (text == 0u) text = kEmptyText;
        if (runtime.ReadByte(text - 1u) < 0xfeu) {
            runtime.SetErrorFlag(destination, 1u);
            text = runtime.ReadWord(source + 4u);
            if (text == 0u) text = kEmptyText;
            runtime.WriteWord(destination + 4u, text);
            runtime.WriteWord(destination + 8u,
                              runtime.ReadWord(source + 8u));
            runtime.WriteWord(destination + 12u,
                              runtime.ReadWord(source + 12u));
            runtime.WriteByte(text - 1u, static_cast<std::uint8_t>(
                runtime.ReadByte(text - 1u) + 1u));
            return;
        }
    }

    if (!runtime.AllocateText(destination, length, 1u)) return;
    auto text = runtime.ReadWord(source + 4u);
    if (text == 0u) text = kEmptyText;
    runtime.CopyBytes(runtime.ReadWord(destination + 4u), text, length);
    runtime.SetTextSize(destination, length);
}

}  // namespace

std::uint32_t ActorManagerCombatActor::CopyFrom(
    const std::uint32_t object, const std::uint32_t source,
    IActorManagerCombatCopyRuntime& runtime) {
    const auto source_records = source + 336u;
    const auto destination_records = object + 896u;
    for (std::uint32_t word = 0u; word < 18u; ++word)
        runtime.WriteWord(destination_records + word * 4u,
                          runtime.ReadWord(source_records + word * 4u));
    for (std::uint32_t index = 0u; index < 6u; ++index)
        CopyTextRecord(destination_records + 72u + index * 16u,
                       source_records + 72u + index * 16u, runtime);
    for (std::uint32_t index = 0u; index < 5u; ++index)
        CopyTextRecord(destination_records + 168u + index * 16u,
                       source_records + 168u + index * 16u, runtime);
    for (std::uint32_t word = 0u; word < 3u; ++word)
        runtime.WriteWord(destination_records + 248u + word * 4u,
                          runtime.ReadWord(source_records + 248u + word * 4u));
    runtime.CopyBytes(object + 4u, source, 336u);
    runtime.CopyBytes(object + 340u, source, 336u);
    return object;
}

}  // namespace pal4
