#include "pal4/arena_actor_resource.h"
#include "pal4/pal_game_iv.h"

#include <bit>

namespace pal4 {

std::uint32_t ArenaActorResource::Construct(
    const std::uint32_t object, IArenaActorRuntime& runtime) {
    (void)runtime.Call(ArenaActorCall::ConstructResourceManager, {object});
    (void)runtime.Call(ArenaActorCall::InitializeObjectState,
                       {object + kObjectStateOffset});
    runtime.WriteWord(object + kObjectStateOffset, kObjectStateVtable);
    runtime.WriteWord(object, kResourceVtable);
    return object;
}

void ArenaActorResource::Destroy(const std::uint32_t object,
                                 IArenaActorRuntime& runtime) {
    runtime.WriteWord(object, kResourceVtable);
    runtime.WriteWord(object + kObjectStateOffset, kObjectStateVtable);
    (void)runtime.Call(ArenaActorCall::CleanupObjectState,
                       {object + kObjectStateOffset});
    (void)runtime.Call(ArenaActorCall::DestroyResourceManager, {object});
}

std::uint32_t ArenaActorResource::Cleanup(
    const std::uint32_t object, IArenaActorRuntime& runtime) {
    (void)runtime.Call(ArenaActorCall::CleanupObjectData,
                       {object + kObjectStateOffset});
    const auto game = runtime.Call(ArenaActorCall::GetGameInstance, {});
    return runtime.Call(ArenaActorCall::CleanupClump,
                        {object, game + PalGameIV::kTextureDictionaryOffset});
}

std::uint32_t ArenaActorResource::RenderAndAnimate(
    const std::uint32_t object, const std::uint32_t pass,
    IArenaActorRuntime& runtime) {
    (void)runtime.Call(ArenaActorCall::RenderClump, {object, pass});
    return runtime.Call(ArenaActorCall::ProcessAnimation,
                        {object + kObjectStateOffset, pass});
}

std::uint32_t ArenaActorResource::Update(
    const float delta, IArenaActorRuntime& runtime) {
    const auto bits = std::bit_cast<std::uint32_t>(delta);
    (void)runtime.Call(ArenaActorCall::UpdateAndRenderClump, {bits});
    return runtime.Call(ArenaActorCall::UpdateInterpolator, {bits});
}

bool ArenaActorResource::ExecuteScriptAndLoad(
    const std::uint32_t object, const std::uint32_t resource_id,
    const std::uint32_t name, const std::uint32_t options,
    const std::uint32_t temporary_name, IArenaActorRuntime& runtime) {
    (void)Cleanup(object, runtime);
    (void)runtime.Call(ArenaActorCall::InitializeName,
                       {temporary_name, name});
    const auto marker = static_cast<std::int32_t>(
        runtime.Call(ArenaActorCall::FindScriptMarker, {temporary_name}));
    if (marker >= 0) {
        (void)runtime.Call(ArenaActorCall::ProcessEmbeddedScript, {});
    } else {
        const auto manager = runtime.Call(ArenaActorCall::GetArenaManager, {});
        if (runtime.ReadWord(manager + 316u) != 0u) {
            const auto current = runtime.Call(ArenaActorCall::GetArenaManager,
                                              {});
            (void)runtime.Call(ArenaActorCall::ProcessArenaScript,
                               {runtime.ReadWord(current + 316u)});
        }
    }
    const auto game = runtime.Call(ArenaActorCall::GetGameInstance, {});
    const bool loaded = runtime.Call(ArenaActorCall::LoadClump,
                                     {object, resource_id, name, options,
                                      game + PalGameIV::kTextureDictionaryOffset,
                                      0u, 0u, 1u}) != 0u;
    if (loaded) {
        const auto clump = runtime.ReadWord(object + 20u);
        (void)runtime.Call(ArenaActorCall::HideUserDataAtomics, {clump});
        (void)runtime.Call(ArenaActorCall::LoadAnimationData,
                           {object + kObjectStateOffset, clump, name, options});
    }
    (void)runtime.Call(ArenaActorCall::CleanupScripts, {});
    (void)runtime.Call(ArenaActorCall::CleanupName, {temporary_name});
    return loaded;
}

}  // namespace pal4
