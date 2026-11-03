#include "pal4/actor_manager_combat_visuals.h"

#include <array>

namespace pal4 {

std::uint32_t ActorManagerCombatVisuals::Handle(
    const std::uint32_t manager, const std::uint32_t parameter,
    IActorManagerCombatVisualRuntime& runtime) {
    if (runtime.CombatInactive(runtime.CombatSystem())) return 1u;

    const auto handle_address = manager + 392u;
    runtime.HandleExistingEffect(handle_address, 1u);
    const auto dialog = runtime.DialogData(parameter);
    const auto variant = runtime.ReadWord(dialog + 1192u);
    constexpr std::array<std::uint32_t, 4> kNames{
        0x8b67a8u, 0x8b67a0u, 0x8b6798u, 0x8b6790u};
    if (variant < kNames.size()) {
        auto created = runtime.CreateEffect(
            manager, kNames[variant], kPosition, 1u, 0u);
        runtime.WriteWord(handle_address, created.id);
        runtime.WriteWord(handle_address + 4u, created.owner);
        runtime.ResetTemporaryEffect(created);
    }

    const auto property_dialog = runtime.DialogData(parameter);
    const auto property_source = property_dialog == 0u
        ? 0u : property_dialog + 724u;
    const auto text = runtime.CreateText(kT1Text);
    try {
        runtime.SetProperties(handle_address, text, property_source);
    } catch (...) {
        if (text != 0u) runtime.ReleaseText(text);
        throw;
    }
    if (text != 0u) runtime.ReleaseText(text);

    const auto position_dialog = runtime.DialogData(parameter);
    const auto position = runtime.ChildPosition(position_dialog);
    runtime.UpdatePosition(handle_address, position, 0u);
    const auto matrix_dialog = runtime.DialogData(parameter);
    const auto matrix = runtime.ChildMatrix(matrix_dialog);
    runtime.UpdateMatrix(handle_address, matrix, 0u);
    runtime.UpdateOrientation(handle_address, {0.0f, 180.0f, 0.0f}, 1u);
    return static_cast<std::uint8_t>(
        runtime.FinalizeEffect(handle_address));
}

void ActorManagerCombatVisuals::ForwardEvent(
    const std::uint32_t manager,
    IActorManagerCombatVisualRuntime& runtime) {
    if (!runtime.CombatInactive(runtime.CombatSystem()))
        runtime.HandleExistingEffect(manager + 392u, 1u);
}

}  // namespace pal4
