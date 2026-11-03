#include "pal4/actor_manager_player_action_playback.h"

#include <string>

namespace pal4 {

std::uint8_t ActorManagerPlayerActionPlayback::Play(
    const std::uint32_t actor, const std::string_view action,
    const std::uint8_t flags,
    IActorManagerPlayerActionRuntime& runtime) {
    const auto current = runtime.ReadWord(actor + 808u);
    if (!current) return 0u;

    std::string filename(action);
    filename += ".anm";
    if (runtime.HasActionAnimation(current, filename)) {
        runtime.PlayAction(actor, action, flags);
        return 0u;
    }

    runtime.TouchSkillSystem();
    runtime.TouchGlobalSystem();
    const auto name_pointer = runtime.ReadWord(actor + 1148u);
    const auto name_length = runtime.ReadWord(actor + 1152u);
    const auto requested = runtime.ReadCString(
        name_pointer ? name_pointer : kEmptyName).substr(0u, name_length);
    const auto active_name = runtime.AnimationName(current);
    if (active_name == requested) {
        std::string replacement_name(active_name);
        replacement_name += action;
        runtime.TouchSkillSystem();
        runtime.TouchGlobalSystem();
        const auto replacement = runtime.FindOrLoad(actor, replacement_name);
        if (replacement) {
            runtime.GetFrame(replacement);
            runtime.CleanupAnimation(replacement, actor);
            runtime.SetTransformFromActor(replacement, current + 172u, 0u);
            runtime.SetTransformFromPosition(replacement, current + 160u, 0u);
            runtime.SetLeftHandActive(replacement, true);
            runtime.SetRightHandActive(replacement, true);
            runtime.SetLeftHandActive(current, false);
            runtime.SetRightHandActive(current, false);
            runtime.WriteWord(actor + 808u, replacement);
            runtime.UpdateAttachments(actor, replacement);
            runtime.DispatchRejoinScene(actor);
        }
    }
    runtime.PlayAction(actor, action, flags);
    return 0u;
}

}  // namespace pal4
