#include "pal4/combat_ending_window_prize_events.h"

namespace pal4 {

std::uint32_t CombatEndingWindowPrizeEvents::Initialize(
    const std::uint32_t window, IEventSubscriptionRuntime& runtime) {
    (void)Cleanup(window, runtime);
    const auto key = runtime.EventKey("combat_prize");
    const auto handler = runtime.Register(
        runtime.EventRouter(), 2u, key, window, kUpdateCallback);
    runtime.WriteWord(window + kKeyOffset, key);
    runtime.WriteWord(window + kHandleOffset, handler);
    return handler;
}

std::uint32_t CombatEndingWindowPrizeEvents::Cleanup(
    const std::uint32_t window, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(window, kHandleOffset, 1u, runtime);
    return 0u;
}

}  // namespace pal4
