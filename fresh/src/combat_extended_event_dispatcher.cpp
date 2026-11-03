#include "pal4/combat_extended_event_dispatcher.h"

namespace pal4 {

void CombatExtendedEventDispatcher::Dispatch(
    std::uint32_t owner, std::uint32_t event,
    ICombatExtendedEventDispatchRuntime& runtime) {
    switch (runtime.ReadWord(event + 12u)) {
    case 0u:
        runtime.SelectMagic(event);
        return;
    case 1u:
        runtime.SelectStunt(event);
        return;
    case 2u:
        runtime.UseProperty(event);
        return;
    case 4u:
        runtime.SelectAttack(event);
        return;
    case 3u:
    case 5u: {
        const auto owner_high = static_cast<std::uint8_t>(owner >> 24u);
        const auto first_router = runtime.ScriptEventRouter();
        runtime.ProcessByteEvent(first_router, 0x8b62d8u,
                                 owner_high, 1u);
        const auto second_router = runtime.ScriptEventRouter();
        runtime.ProcessRecordEvent(second_router, 0x8b62ccu,
                                   event, 1u);
        return;
    }
    default:
        return;
    }
}

}  // namespace pal4
