#include "pal4/combat_event_state_updater.h"

#include <array>
#include <bit>

namespace pal4 {

std::int32_t CombatEventStateUpdater::Process(
    std::uint32_t actor, std::int32_t value,
    std::uint32_t key, ICombatEventStateUpdateRuntime& runtime,
    ICombatEventDataRuntime& event_runtime) {
    runtime.WriteWord(actor + 224u, std::bit_cast<std::uint32_t>(value));
    static_cast<void>(runtime.SetSlot(actor, 4u, value));

    std::array<std::uint32_t, 5> cached{};
    std::array<std::int32_t, 5> slots{};
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        cached[index] = runtime.ReadWord(actor + 128u + index * 4u);
        slots[index] = runtime.QuerySlot(actor, index);
    }

    static_cast<void>(CombatEventDataProcessor::Process(
        actor + 4u, key + 1001u, std::bit_cast<std::uint32_t>(value),
        event_runtime));

    for (std::uint32_t index = 0u; index < 5u; ++index) {
        runtime.WriteWord(actor + 128u + index * 4u, cached[index]);
        runtime.RestoreSlot(actor, index, slots[index]);
    }
    runtime.Notify(actor);
    static_cast<void>(runtime.SetSlot(actor, 7u, runtime.BaseStat(actor, 0u)));
    return runtime.SetSlot(actor, 9u, runtime.BaseStat(actor, 2u));
}

}  // namespace pal4
