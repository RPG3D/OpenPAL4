#include "pal4/actor_manager_effect_events.h"

namespace pal4 {

namespace {

void CleanupTemporaryPayload(const std::uint32_t payload,
                             IActorManagerEffectEventRuntime& runtime) {
    const auto buffer = runtime.ReadWord(payload + 12u);
    if (buffer != 0u) {
        const auto count = runtime.ReadByte(buffer - 1u);
        if (count == 0u || count == 0xffu)
            runtime.Free(buffer - 1u);
        else
            runtime.WriteByte(buffer - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    runtime.WriteWord(payload + 12u, 0u);
    runtime.WriteWord(payload + 16u, 0u);
    runtime.WriteWord(payload + 20u, 0u);
    runtime.WriteWord(payload, 0xffffffffu);
    runtime.WriteWord(payload + 4u, 0u);
}

}  // namespace

std::uint32_t ActorManagerEffectEvents::HandleEffectEnd(
    const std::uint32_t manager, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IActorManagerEffectEventRuntime& runtime) {
    if (runtime.ReadWord(manager + 384u) == first &&
        runtime.ReadWord(manager + 388u) == second && third == 0u) {
        runtime.WriteWord(runtime.CombatSystem() + 508u, 2u);
        const auto key = runtime.ReadWord(kCombatStartKey);
        runtime.DispatchWord(runtime.EventRouter(), key, third, 1u);
    }

    const auto payload = DNameNodeEventRecord::SetString(
        runtime.TemporaryPayload(), first, second, kEmptyText, runtime);
    const auto key = runtime.ReadWord(kEffectEndKey);
    runtime.DispatchAddress(runtime.EventRouter(), key, payload, 1u);
    CleanupTemporaryPayload(payload, runtime);
    return runtime.ResetArgumentPair(first, second);
}

std::uint32_t ActorManagerEffectEvents::HandleEffectMessage(
    std::uint32_t first, std::uint32_t second,
    const std::uint32_t text,
    IActorManagerEffectEventRuntime& runtime) {
    const auto payload = DNameNodeEventRecord::SetString(
        runtime.TemporaryPayload(), first, second, text, runtime);
    const auto key = runtime.ReadWord(kEffectMessageKey);
    runtime.DispatchAddress(runtime.EventRouter(), key, payload, 1u);
    CleanupTemporaryPayload(payload, runtime);
    return runtime.ResetArgumentPair(first, second);
}

}  // namespace pal4
