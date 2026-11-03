#include "pal4/combat_event_lifecycle.h"

namespace pal4 {

std::uint32_t CombatEventLifecycle::InitializeRouter(
    std::uint32_t router, ICombatEventLifecycleRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 67u; ++index) {
        runtime.WriteWord(router + index * 8u, 0u);
        runtime.WriteWord(router + index * 8u + 4u, 0u);
    }
    runtime.InitializeEmptyString(router + 536u);
    return router;
}

std::uint32_t CombatEventLifecycle::CleanupRouter(
    std::uint32_t router, ICombatEventLifecycleRuntime& runtime) {
    return runtime.CleanupString(router + 536u);
}

}  // namespace pal4
