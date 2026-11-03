#include "pal4/combat_event_data_registry.h"

namespace pal4 {

std::uint32_t CombatEventDataRegistry::Lookup(
    std::uint32_t index, ICombatEventDataRegistryRuntime& runtime) {
    if (index >= kCount) return 0u;
    return runtime.ReadWord(kBase + index * 4u);
}

}  // namespace pal4
