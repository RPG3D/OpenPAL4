#include "pal4/combat_dialog_property_callbacks.h"

#include <stdexcept>

namespace pal4 {

std::uint8_t CombatDialogPropertyCallbacks::HandlePrimary(
    const std::uint32_t event, ICombatDialogPropertyRuntime& runtime) {
    return Handle(event, 3u, runtime);
}

std::uint8_t CombatDialogPropertyCallbacks::HandleExtended(
    const std::uint32_t event, ICombatDialogPropertyRuntime& runtime) {
    return Handle(event, 6u, runtime);
}

std::uint8_t CombatDialogPropertyCallbacks::Handle(
    const std::uint32_t event, const std::uint32_t capacity,
    ICombatDialogPropertyRuntime& runtime) {
    const auto actor = runtime.ActorFromFacet(event);
    const auto current = runtime.ActorSlot(actor);
    std::array<std::uint32_t, 6> properties{};
    runtime.GetProperties(actor, properties);
    if (current < 0) throw std::out_of_range("negative actor property slot");
    auto slot = static_cast<std::uint32_t>(current);
    if (slot >= capacity || properties[slot] != 0u) {
        slot = 0u;
        while (slot < capacity && properties[slot] != 0u)
            ++slot;
        if (slot == capacity)
            return static_cast<std::uint8_t>(capacity);
    }
    return runtime.InitializeEffects(actor, slot);
}

}  // namespace pal4
