#include "pal4/actor_manager_selection.h"

namespace pal4 {

std::uint32_t ActorManagerSelection::SetSelected(
    const std::uint32_t actor, const std::uint32_t selected,
    IActorManagerSelectionVisualRuntime& runtime) {
    if (static_cast<std::uint8_t>(selected)) {
        std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
        runtime.AdjustScale(actor, scale);
        runtime.UpdateDialog(actor + 852u);
        const auto dialog = runtime.BuildDialog(actor, "pick", 0x8fa274u);
        runtime.WriteWord(actor + 852u, dialog.first);
        runtime.WriteWord(actor + 856u, dialog.second);
        runtime.ResetTemporaryHandle();
        runtime.UpdateEffectProperties(actor + 852u, scale, 0u);

        const auto box = runtime.ActorBoundingBox(actor);
        auto position = runtime.ActorPosition(actor);
        position[1] += box[7] + 20.0f;
        runtime.HandleEffect(actor + 860u, actor, false);
        const auto effect = runtime.CreateEffect(
            runtime.CombatCamera(), "OM03", position, 1u, 0u);
        runtime.WriteWord(actor + 860u, effect.first);
        runtime.WriteWord(actor + 864u, effect.second);
        runtime.ResetTemporaryHandle();
        return runtime.Dispatch("ae_ui_select", actor, 1u);
    }
    runtime.UpdateDialog(actor + 852u);
    runtime.HandleEffect(actor + 860u, actor, false);
    return runtime.Dispatch("ae_ui_unselect", actor, 1u);
}

double ActorManagerSelection::CriticalRatio(
    const std::uint32_t facet, IActorManagerActionProgressRuntime& runtime) {
    return 64.0 / runtime.CriticalDamageMultiplier(facet - 716u);
}

bool ActorManagerSelection::IsStateOne(
    const std::uint32_t facet, IActorManagerActionProgressRuntime& runtime) {
    const auto state = runtime.ReadWord(facet + 112u);
    if (state <= 7u) runtime.TouchSkillSystem();
    return state == 1u;
}

}  // namespace pal4
