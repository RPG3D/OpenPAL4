#include "pal4/game_arena_release.h"
#include "pal4/pal_game_iv.h"

#include <string>

namespace pal4 {
namespace {

using Call = GameArenaReleaseCall;

std::uint32_t Invoke(IGameArenaReleaseRuntime& runtime, const Call operation,
                     const std::uint32_t first = 0u,
                     const std::uint32_t second = 0u,
                     const std::uint32_t third = 0u) {
    return runtime.Call(operation, first, second, third);
}

std::string CopyCString(const std::string_view value) {
    return std::string(value.substr(0, value.find('\0')));
}

}  // namespace

bool GameArenaRelease::Release(const std::uint32_t arena,
                               IGameArenaReleaseRuntime& runtime) {
    const auto script = runtime.ReadWord(arena + 316u);
    if (script != 0u) {
        Invoke(runtime, Call::DestroyScript, script);
        runtime.WriteWord(arena + 316u, 0u);
    }
    if (runtime.ReadWord(arena + 48u) == 0u) return false;

    ReleaseEffectsAndRecords(arena, runtime);

    runtime.StoreText(arena + 240u,
                      CopyCString(runtime.ReadCString(arena + 8u)));
    runtime.StoreText(arena + 252u,
                      CopyCString(runtime.ReadCString(arena + 12u)));
    Invoke(runtime, Call::CleanupLogHandlers,
           Invoke(runtime, Call::GetScriptExecutor) + 0x54u);
    Invoke(runtime, Call::ResetScriptExecutor,
           Invoke(runtime, Call::GetScriptExecutor));
    Invoke(runtime, Call::CleanupCameraTree,
           Invoke(runtime, Call::GetScriptCloudManager));
    Invoke(runtime, Call::CleanupCameraProperties,
           Invoke(runtime, Call::GetCameraSubsystem));

    ReleaseRenderBindings(arena, runtime);
    ReleaseMovementAndSceneResources(arena,runtime);
    Invoke(runtime, Call::ResetObjectList, arena);

    const auto auxiliary = runtime.ReadWord(arena + 404u);
    if (auxiliary != 0u) {
        Invoke(runtime, Call::CleanupAuxObjectFirst, auxiliary);
        const auto current = runtime.ReadWord(arena + 404u);
        if (current != 0u) {
            Invoke(runtime, Call::CleanupAuxObjectSecond, current);
            Invoke(runtime, Call::FreeMemory, current);
        }
        runtime.WriteWord(arena + 404u, 0u);
    }

    Invoke(runtime, Call::ClearActor,
           Invoke(runtime, Call::GetActorControl));
    Invoke(runtime, Call::ForAllPlayers,
           Invoke(runtime, Call::GetPlayerManager), arena);
    Invoke(runtime, Call::UpdatePlayers,
           Invoke(runtime, Call::GetPlayerManager));
    Invoke(runtime, Call::RemovePetScript, arena);
    Invoke(runtime, Call::CleanupPets,
           Invoke(runtime, Call::GetPetManager));
    ReleaseObjectManagers(arena, runtime);
    ReleaseWorldAndPipeline(arena, runtime);
    return true;
}

void GameArenaRelease::ReleaseWorldAndPipeline(
    const std::uint32_t arena, IGameArenaReleaseRuntime& runtime) {
    Invoke(runtime, Call::CleanupWorld, arena,
           Invoke(runtime, Call::GetGameController) +
               PalGameIV::kTextureDictionaryOffset);

    const auto pipeline = runtime.ReadWord(arena + 424u);
    runtime.WriteWord(arena + 264u, 0u);
    if (pipeline != 0u) {
        Invoke(runtime, Call::DestroyPipeline, pipeline);
        runtime.WriteWord(arena + 424u, 0u);
    }
}

void GameArenaRelease::ReleaseObjectManagers(
    const std::uint32_t arena, IGameArenaReleaseRuntime& runtime) {
    Invoke(runtime, Call::ForAllNpcs,
           Invoke(runtime, Call::GetNpcManager), arena);
    Invoke(runtime, Call::CleanupNpcs,
           Invoke(runtime, Call::GetNpcManager));
    Invoke(runtime, Call::ForAllMonsters,
           Invoke(runtime, Call::GetMonsterManager), arena);
    Invoke(runtime, Call::CleanupMonsters,
           Invoke(runtime, Call::GetMonsterManager));
    Invoke(runtime, Call::ForAllObjects,
           Invoke(runtime, Call::GetObjectManager), arena);
    Invoke(runtime, Call::CleanupObjects,
           Invoke(runtime, Call::GetObjectManager));
    Invoke(runtime, Call::ForAllSecondaryObjects,
           Invoke(runtime, Call::GetSecondaryObjectManager), arena);
    Invoke(runtime, Call::CleanupSecondaryObjects,
           Invoke(runtime, Call::GetSecondaryObjectManager));
    Invoke(runtime, Call::CleanupPaths,
           Invoke(runtime, Call::GetPathPlanner));
}

void GameArenaRelease::ReleaseEffectsAndRecords(
    const std::uint32_t arena, IGameArenaReleaseRuntime& runtime) {
    const auto defaults = runtime.ReadWord(arena + 504u);
    if (defaults != 0u) {
        Invoke(runtime, Call::ResetDefaults, defaults);
        const auto current = runtime.ReadWord(arena + 504u);
        if (current != 0u) {
            Invoke(runtime, Call::DestroyDefaults, current);
            Invoke(runtime, Call::FreeMemory, current);
        }
        runtime.WriteWord(arena + 504u, 0u);
    }

    Invoke(runtime, Call::UpdateWeatherEffects, arena);
    Invoke(runtime, Call::ShutdownEffectSingleton,
           Invoke(runtime, Call::GetEffectSingleton));
    Invoke(runtime, Call::CleanupEffects,
           Invoke(runtime, Call::GetEffectSystem));
    for (const auto offset : {616u, 632u, 648u, 664u, 680u}) {
        const auto begin = runtime.ReadWord(arena + offset + 4u);
        const auto end = runtime.ReadWord(arena + offset + 8u);
        static_cast<void>(GameArenaRecordCompactor::Compact(
            arena + offset, begin, end, runtime));
    }
    Invoke(runtime, Call::ClearArenaSkillObjects,
           Invoke(runtime, Call::GetArenaSkillManager));
}

void GameArenaRelease::ReleaseRenderBindings(const std::uint32_t arena,
                                            IGameArenaReleaseRuntime& runtime) {
    for (const auto offset : {52u, 72u, 92u}) {
        const auto clump = runtime.ReadWord(arena + offset);
        if (clump == 0u) continue;
        Invoke(runtime, Call::RemoveClump, arena, clump);
        if (offset != 92u)
            Invoke(runtime, Call::ResetClumpMaterials,
                   runtime.ReadWord(arena + offset));
    }
    for (const auto offset : {268u, 284u, 300u})
        Invoke(runtime, Call::CleanupGeometry, arena + offset);
}

void GameArenaRelease::ReleaseMovementAndSceneResources(
    const std::uint32_t arena,IGameArenaReleaseRuntime& runtime) {
    for (const auto offset : {0u,4u}) {
        const auto adapter=Invoke(runtime,Call::GetMovementAdapter);
        Invoke(runtime,Call::CleanupMovementComponent,runtime.ReadWord(adapter+offset));
    }
    Invoke(runtime,Call::CleanupCache,arena+448u);
    Invoke(runtime,Call::CleanupConfig,arena+320u);
}

}  // namespace pal4
