#include "pal4/actor_manager_combat_preview.h"

namespace pal4 {

namespace {

std::uint32_t HandleDialogPositionEffect(
    const std::uint32_t manager, const std::uint32_t parameter,
    const std::uint32_t name, const bool truncate_result,
    IActorManagerCombatPreviewRuntime& runtime) {
    const auto actor = runtime.DialogData(parameter);
    if (runtime.ReadWord(actor + 804u) != 2u)
        return truncate_result ? static_cast<std::uint8_t>(actor) : actor;

    auto effect = runtime.CreateEffect(
        manager, name, ActorManagerCombatPreview::kPosition, 1u, 0u);
    try {
        runtime.UpdatePosition(effect,
                               runtime.ChildPosition(runtime.DialogData(parameter)),
                               0u);
        runtime.FinalizeEffect(effect);
    } catch (...) {
        runtime.ResetTemporaryEffect(effect);
        throw;
    }
    runtime.ResetTemporaryEffect(effect);
    return 0u;
}

}  // namespace

std::uint32_t ActorManagerCombatPreview::Handle(
    const std::uint32_t manager, const std::uint32_t event,
    IActorManagerCombatPreviewRuntime& runtime) {
    if (runtime.CombatInactive(runtime.CombatSystem())) return 1u;

    auto effect = runtime.CreateEffect(manager, kCA46, kPosition, 1u, 0u);
    try {
        if (runtime.NeedsUpdate(effect)) {
            const auto actor = runtime.ReadWord(event + 8u);
            const auto source = actor == 0u ? 0u : actor + 724u;
            const auto text = runtime.CreateText(kT1Text);
            try {
                runtime.SetProperties(effect, text, source);
            } catch (...) {
                if (text != 0u) runtime.ReleaseText(text);
                throw;
            }
            if (text != 0u) runtime.ReleaseText(text);

            runtime.UpdatePosition(effect,
                                   runtime.ChildPosition(runtime.ReadWord(event + 8u)),
                                   0u);
            runtime.UpdateMatrix(effect,
                                 runtime.ChildMatrix(runtime.ReadWord(event + 8u)),
                                 0u);
            runtime.UpdateOrientation(effect, {0.0f, 180.0f, 0.0f}, 1u);
            runtime.FinalizeEffect(effect);
        }
    } catch (...) {
        runtime.ResetTemporaryEffect(effect);
        throw;
    }
    runtime.ResetTemporaryEffect(effect);
    // The original returns an incidental register value after the void reset.
    return 0u;
}

std::uint32_t ActorManagerCombatPreview::HandleEff4(
    const std::uint32_t manager, const std::uint32_t event,
    IActorManagerCombatPreviewRuntime& runtime) {
    if (runtime.CombatInactive(runtime.CombatSystem())) return 1u;
    const auto actor = runtime.ReadWord(event + 4u);
    const auto state = runtime.ReadWord(actor + 804u);
    if (state == 1u) return state;

    auto effect = runtime.CreateEffect(manager, kEFF4, kPosition, 1u, 0u);
    try {
        runtime.UpdatePosition(effect,
                               runtime.ChildPosition(runtime.ReadWord(event + 4u)),
                               0u);
        runtime.UpdateMatrix(effect,
                             runtime.ChildMatrix(runtime.ReadWord(event + 4u)),
                             0u);
        runtime.FinalizeEffect(effect);
    } catch (...) {
        runtime.ResetTemporaryEffect(effect);
        throw;
    }
    runtime.ResetTemporaryEffect(effect);
    return 0u;
}

std::uint32_t ActorManagerCombatPreview::HandleEff3(
    const std::uint32_t manager, const std::uint32_t event,
    IActorManagerCombatPreviewRuntime& runtime) {
    if (runtime.CombatInactive(runtime.CombatSystem())) return 1u;

    auto effect = runtime.CreateEffect(manager, kEFF3, kPosition, 1u, 0u);
    try {
        runtime.UpdatePosition(effect,
                               runtime.ChildPosition(runtime.ReadWord(event + 4u)),
                               0u);
        runtime.FinalizeEffect(effect);
    } catch (...) {
        runtime.ResetTemporaryEffect(effect);
        throw;
    }
    runtime.ResetTemporaryEffect(effect);
    return 0u;
}

std::uint32_t ActorManagerCombatPreview::HandleEff7(
    const std::uint32_t manager, const std::uint32_t parameter,
    IActorManagerCombatPreviewRuntime& runtime) {
    if (runtime.CombatInactive(runtime.CombatSystem())) return 1u;
    if (!runtime.CombatStateActive(runtime.CombatSystem())) return 0u;
    return HandleDialogPositionEffect(manager, parameter, kEFF7, true, runtime);
}

std::uint32_t ActorManagerCombatPreview::HandleEff6(
    const std::uint32_t manager, const std::uint32_t parameter,
    IActorManagerCombatPreviewRuntime& runtime) {
    if (runtime.CombatInactive(runtime.CombatSystem())) return 1u;
    return HandleDialogPositionEffect(manager, parameter, kEFF6, false, runtime);
}

}  // namespace pal4
