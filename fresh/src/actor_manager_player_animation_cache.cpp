#include "pal4/actor_manager_player_animation_cache.h"

namespace pal4 {

namespace {

bool AppendAnimation(const std::uint32_t actor,
                     const std::uint32_t animation,
                     IActorManagerPlayerAnimationRuntime& runtime) {
    const auto current_begin = runtime.ReadWord(actor + 1296u);
    const auto current_end = runtime.ReadWord(actor + 1300u);
    const auto capacity_end = runtime.ReadWord(actor + 1304u);
    if (capacity_end >= current_end && capacity_end - current_end >= 4u) {
        runtime.WriteWord(current_end, animation);
        runtime.WriteWord(actor + 1300u, current_end + 4u);
        return true;
    }

    const auto size = current_begin ?
        (current_end - current_begin) / 4u : 0u;
    const auto growth = size > 1u ? size : 1u;
    const auto new_capacity = size + growth;
    const auto replacement = runtime.AllocatePointerArray(new_capacity * 4u);
    if (!replacement) return false;
    for (std::uint32_t i = 0u; i < size; ++i)
        runtime.WriteWord(replacement + i * 4u,
                          runtime.ReadWord(current_begin + i * 4u));
    runtime.WriteWord(replacement + size * 4u, animation);
    runtime.FreePointerArray(current_begin);
    runtime.WriteWord(actor + 1304u, replacement + new_capacity * 4u);
    runtime.WriteWord(actor + 1296u, replacement);
    runtime.WriteWord(actor + 1300u, replacement + (size + 1u) * 4u);
    return true;
}

}  // namespace

std::uint32_t ActorManagerPlayerAnimationCache::Activate(
    const std::uint32_t actor,
    IActorManagerPlayerAnimationRuntime& runtime) {
    auto name = runtime.ReadWord(actor + 1148u);
    if (!name) name = kEmptyName;
    const auto animation = FindOrLoad(actor, name, runtime);
    runtime.WriteWord(actor + 808u, animation);
    if (!runtime.ReadWord(actor + 808u)) return animation;

    runtime.GetFrame(runtime.ReadWord(actor + 808u));
    runtime.RenderCamera(runtime.ReadWord(actor + 808u), 0u, 0u);
    runtime.CleanupAnimation(runtime.ReadWord(actor + 808u), actor);
    return runtime.ProcessAnimationEvent(actor,
                                         runtime.ReadWord(actor + 808u));
}

std::uint32_t ActorManagerPlayerAnimationCache::FindOrLoad(
    const std::uint32_t actor, const std::uint32_t name,
    IActorManagerPlayerAnimationRuntime& runtime) {
    const auto requested = runtime.ReadCString(name);
    const auto begin = runtime.ReadWord(actor + 1296u);
    const auto end = runtime.ReadWord(actor + 1300u);
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        const auto animation = runtime.ReadWord(cursor);
        if (runtime.AnimationName(animation) == requested)
            return animation;
    }

    const auto manager = runtime.SingletonManager();
    const auto animation = runtime.LoadAnimation(manager + 64u, name);
    if (!animation) return 0u;
    return AppendAnimation(actor, animation, runtime) ? animation : 0u;
}

void ActorManagerPlayerAnimationCache::SwitchToRequestedAnimation(
    const std::uint32_t actor,
    IActorManagerPlayerAnimationRuntime& runtime) {
    const auto current = runtime.ReadWord(actor + 808u);
    const auto name_pointer = runtime.ReadWord(actor + 1148u);
    const auto name_length = runtime.ReadWord(actor + 1152u);
    const auto requested = runtime.ReadCString(
        name_pointer ? name_pointer : kEmptyName).substr(0u, name_length);
    if (runtime.AnimationName(current) == requested) return;

    std::uint32_t replacement = 0u;
    bool loaded = false;
    const auto begin = runtime.ReadWord(actor + 1296u);
    const auto end = runtime.ReadWord(actor + 1300u);
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        const auto candidate = runtime.ReadWord(cursor);
        if (runtime.AnimationName(candidate) == requested) {
            replacement = candidate;
            break;
        }
    }

    if (!replacement) {
        const auto manager = runtime.SingletonManager();
        replacement = runtime.LoadAnimation(
            manager + 64u, name_pointer ? name_pointer : kEmptyName);
        // This call site inserts even a null loader result.
        if (!AppendAnimation(actor, replacement, runtime)) return;
        loaded = true;
    }
    if (!replacement || replacement == current) return;

    runtime.SetTransformFromActor(replacement, current + 172u, 0u);
    runtime.SetTransformFromPosition(replacement, current + 160u, 0u);
    runtime.SetLeftHandActive(replacement, true);
    runtime.SetRightHandActive(replacement, true);
    runtime.SetLeftHandActive(current, false);
    runtime.SetRightHandActive(current, false);
    runtime.WriteWord(actor + 808u, replacement);
    if (loaded) runtime.DispatchRejoinScene(actor);
}

}  // namespace pal4
