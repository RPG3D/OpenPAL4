#include "pal4/actor_manager_effect_attachment.h"

namespace pal4 {

ActorEffectHandle ActorManagerEffectAttachment::Create(
    std::uint32_t actor, std::string_view resource,
    std::string_view mode, IActorManagerEffectAttachmentRuntime& runtime) {
    const auto clump = runtime.ChildClump(actor);
    // 52E317 constructs a DNameNode: invalid ID, null owner.
    if (clump == 0u) return {0xffffffffu, 0u};

    const auto effect = runtime.CreateEffect(
        runtime.CombatEffectGroup(), resource, {0.0f, 1.0f, 1.0f});
    if (runtime.IsValid(effect)) {
        if (runtime.MatchesPrimaryMode(mode)) {
            runtime.SetVectorProperty(effect,
                                      runtime.ActorSlot448Position(actor),
                                      false, 23u);
        } else if (runtime.MatchesSecondaryMode(mode)) {
            auto position = runtime.ChildPosition(actor);
            position[1] = 2.0f;
            runtime.SetFacetProperty(effect, "T1", actor + 724u);
            runtime.SetVectorProperty(effect, position, false, 23u);
            runtime.SetRotationVectorProperty(effect, runtime.ChildRotationVector(actor),
                                      false);
        } else {
            runtime.SetVectorProperty(effect, {90.0f, 0.0f, 0.0f}, true, 21u);
            runtime.SetFacetProperty(effect, "T1", actor + 724u);
            runtime.ProcessClumpUserData(clump, mode,
                                         runtime.EffectFrame(effect));
        }
        runtime.EnableTasks(effect);
        const auto event_effect = runtime.CopyEffect(effect);
        runtime.DispatchAttachEffect(actor, event_effect);
        runtime.ResetTemporary(event_effect);
    }
    const auto result = runtime.CopyEffect(effect);
    runtime.ResetTemporary(effect);
    return result;
}

void ActorManagerEffectAttachment::ResetEmbedded(
    std::uint32_t object, IActorManagerEffectAttachmentRuntime& runtime) {
    runtime.ResetHandleAt(object + 4u);
}

}  // namespace pal4
