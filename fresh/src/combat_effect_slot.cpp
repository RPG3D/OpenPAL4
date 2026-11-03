#include "pal4/combat_effect_slot.h"

namespace pal4 {

std::uint32_t CombatEffectSlot::Construct(
    const std::uint32_t slot, ICombatEffectSlotRuntime& runtime) {
    runtime.ConstructNameNode(slot);
    // The native constructor copies an unwritten stack byte here.
    runtime.WriteByte(slot + 8u, 0u);
    runtime.WriteWord(slot + 12u, 0u);
    runtime.WriteWord(slot + 16u, 0u);
    runtime.WriteWord(slot + 20u, 0u);
    return slot;
}

}  // namespace pal4
