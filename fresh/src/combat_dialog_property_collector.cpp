#include "pal4/combat_dialog_property_collector.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t CombatDialogPropertyCollector::Collect(
    std::array<std::uint32_t, 6>& properties, std::uint32_t actor,
    ICombatDialogPropertyCollectionRuntime& runtime) {
    properties.fill(0u);
    const auto iterator = runtime.CreatePropertyIterator(actor);
    std::uint32_t result = 0u;
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto property = runtime.IteratorCurrent(iterator);
            const auto first_slot = runtime.PropertySlot(property);
            if (first_slot < 0)
                throw std::out_of_range("negative property slot");
            if (first_slot < 6) {
                const auto second_slot = runtime.PropertySlot(property);
                if (second_slot < 0 || second_slot >= 6)
                    throw std::out_of_range("property slot changed out of range");
                properties[static_cast<std::size_t>(second_slot)] = property;
            }
            runtime.IteratorAdvance(iterator);
        }
    } catch (...) {
        if (iterator != 0u)
            (void)runtime.DestroyIterator(iterator);
        throw;
    }
    if (iterator != 0u)
        result = runtime.DestroyIterator(iterator);
    return result;
}

}  // namespace pal4
