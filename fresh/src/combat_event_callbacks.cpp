#include "pal4/combat_event_callbacks.h"

namespace pal4 {

void CombatEventCallbacks::ResetState(
    const std::uint32_t owner, ICombatEventCallbackRuntime& runtime) {
    runtime.WriteByte(owner + 136u, 0u);
}

std::uint32_t CombatEventCallbacks::HandleEvent(
    const std::uint32_t owner, const std::uint32_t event,
    ICombatEventCallbackRuntime& runtime) {
    ResetState(owner, runtime);
    const auto type = runtime.ReadWord(event);
    if (type == 1u || type == 2u) {
        const auto iterator = runtime.BeginActors(type);
        try {
            while (!runtime.AtEnd(iterator)) {
                runtime.ProcessActor(runtime.Actor(iterator));
                runtime.Advance(iterator);
            }
        } catch (...) {
            runtime.DestroyIterator(iterator);
            throw;
        }
        runtime.DestroyIterator(iterator);
    }

    const auto system = runtime.CombatSystemInstance();
    const auto value = runtime.ReadWord(system + 440u);
    if (static_cast<std::int32_t>(value) <= 1) return system;
    const auto text = runtime.FormatHint(runtime.GameString(0x100u), value);
    try {
        runtime.CreateHintWindow();
        runtime.ShowHint(text, 0.5f, 0.2f, 4.0f, 0u, 0u);
    } catch (...) {
        static_cast<void>(runtime.ReleaseHint(text));
        throw;
    }
    return runtime.ReleaseHint(text);
}

}  // namespace pal4
