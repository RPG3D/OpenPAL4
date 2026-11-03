#include "pal4/actor_manager_player_stage_lifecycle.h"

namespace pal4 {

std::uint32_t ActorManagerPlayerStageLifecycle::Enter(
    const std::uint32_t actor, const std::uint32_t context,
    IActorManagerPlayerStageRuntime& runtime) {
    runtime.PrepareStage(actor);
    const auto animation = runtime.ReadWord(actor + 808u);
    if (animation) {
        const auto transform = runtime.ResolveAnimationTransform(actor, 0u,
                                                                 context);
        runtime.SetAnimationTransform(runtime.ReadWord(actor + 808u),
                                      transform);
    }
    static_cast<void>(runtime.SendEvent("re_join_stage", actor));
    if (!runtime.ReadWord(actor + 808u)) return 0u;
    return runtime.SendEvent("re_join_scene", actor);
}

std::uint32_t ActorManagerPlayerStageLifecycle::Leave(
    const std::uint32_t actor,
    IActorManagerPlayerStageRuntime& runtime) {
    static_cast<void>(runtime.SendEvent("re_leave_stage", actor));
    const auto begin = runtime.ReadWord(actor + 1296u);
    auto end = runtime.ReadWord(actor + 1300u);
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        runtime.WriteWord(actor + 808u, runtime.ReadWord(cursor));
        static_cast<void>(runtime.SendEvent("re_leave_scene", actor));
        end = runtime.ReadWord(actor + 1300u);
    }
    runtime.WriteWord(actor + 808u, 0u);
    return end;
}

std::uint32_t ActorManagerPlayerStageLifecycle::LeaveAndReturnAnimations(
    const std::uint32_t actor,
    IActorManagerPlayerStageRuntime& runtime) {
    const auto begin = runtime.ReadWord(actor + 1296u);
    auto end = runtime.ReadWord(actor + 1300u);
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        runtime.WriteWord(actor + 808u, runtime.ReadWord(cursor));
        static_cast<void>(runtime.SendEvent("re_leave_scene", actor));
        const auto manager = runtime.SingletonManager();
        runtime.ReturnAnimationToPool(manager + 64u, 12u,
                                      runtime.ReadWord(actor + 808u));
        end = runtime.ReadWord(actor + 1300u);
    }
    runtime.WriteWord(actor + 808u, 0u);
    return end;
}

}  // namespace pal4
