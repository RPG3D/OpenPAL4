#include "pal4/combat_command_playback.h"

namespace pal4 {
namespace {

void PrepareEvent(std::uint32_t event, std::uint32_t action,
                  ICombatCommandPlaybackRuntime& runtime) {
    if (runtime.ReadWord(event + 12u) != action)
        runtime.WriteWord(event + 12u, action);
    const auto router = runtime.ScriptEventRouter();
    runtime.DispatchEvent(router, 0x8b62d8u, event, 1u, true);
}

void DispatchTarget(std::uint32_t event, std::uint32_t descriptor,
                    ICombatCommandPlaybackRuntime& runtime) {
    const auto router = runtime.ScriptEventRouter();
    runtime.DispatchEvent(router, descriptor, event, 1u, false);
}

}  // namespace

void CombatCommandPlayback::PlayMagic(
    std::uint32_t event, ICombatCommandPlaybackRuntime& runtime) {
    PrepareEvent(event, 0u, runtime);
    const auto database = runtime.GameDatabase();
    const auto magic = runtime.FindMagic(database + 12u,
                                         runtime.ReadWord(event + 8u));
    if (magic != 0u)
        PostProcess(event, runtime.MagicTargetMode(magic), runtime);
}

void CombatCommandPlayback::PlayStunt(
    std::uint32_t event, ICombatCommandPlaybackRuntime& runtime) {
    PrepareEvent(event, 1u, runtime);
    const auto database = runtime.GameDatabase();
    const auto stunt = runtime.FindStunt(database + 12u,
                                         runtime.ReadWord(event + 8u));
    if (stunt != 0u)
        PostProcess(event, runtime.StuntTargetMode(stunt), runtime);
}

void CombatCommandPlayback::UseProperty(
    std::uint32_t event, ICombatCommandPlaybackRuntime& runtime) {
    PrepareEvent(event, 2u, runtime);
    const auto database = runtime.GameDatabase();
    const auto property = runtime.FindProperty(database,
                                                runtime.ReadWord(event + 8u));
    if (property != 0u)
        PostProcess(event, runtime.PropertyTargetMode(property), runtime);
}

void CombatCommandPlayback::PostProcess(
    std::uint32_t event, std::uint32_t target_mode,
    ICombatCommandPlaybackRuntime& runtime) {
    const auto actor = runtime.ActorFromFacet(event);
    switch (target_mode) {
    case 0u:
    case 3u:
        runtime.WriteWord(event + 4u, actor);
        DispatchTarget(event, 0x8b62ccu, runtime);
        return;
    case 1u:
    case 2u:
        DispatchTarget(event, 0x8b8b74u, runtime);
        return;
    case 4u: {
        const auto selected_actor = runtime.ActorFromFacet(event);
        const auto candidates = runtime.SelectCandidates(selected_actor, event);
        try {
            if (runtime.CandidateCount(candidates) > 0) {
                const auto candidate = runtime.CurrentCandidate(candidates);
                if (candidate != 0u) {
                    runtime.WriteWord(event + 4u, candidate);
                    DispatchTarget(event, 0x8b62ccu, runtime);
                }
            }
        } catch (...) {
            if (candidates != 0u)
                runtime.DestroyCandidates(candidates);
            throw;
        }
        if (candidates != 0u)
            runtime.DestroyCandidates(candidates);
        return;
    }
    default:
        runtime.ReportInvalidTargetMode(target_mode);
        return;
    }
}

void CombatCommandPlayback::SelectAttack(
    std::uint32_t event, ICombatCommandPlaybackRuntime& runtime) {
    const auto router = runtime.ScriptEventRouter();
    runtime.DispatchLocalByte(router, 0x8b62d8u, 1u);
    const auto actor = runtime.ActorFromFacet(event);
    if (runtime.ActorAttackMode(actor) == 1u)
        PostProcess(event, 4u, runtime);
    else
        DispatchTarget(event, 0x8b8b74u, runtime);
}

}  // namespace pal4
