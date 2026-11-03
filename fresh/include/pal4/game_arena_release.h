#pragma once

#include "pal4/game_arena_record_compactor.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class GameArenaReleaseCall : std::uint8_t {
    DestroyScript,
    ResetDefaults,
    DestroyDefaults,
    FreeMemory,
    UpdateWeatherEffects,
    GetEffectSingleton,
    ShutdownEffectSingleton,
    GetEffectSystem,
    CleanupEffects,
    GetArenaSkillManager,       // IDA520F30, distinct from the arena manager.
    ClearArenaSkillObjects,     // IDA521370: live, queued and fading records.
    GetScriptExecutor,
    CleanupLogHandlers,
    ResetScriptExecutor,
    GetScriptCloudManager,
    CleanupCameraTree,
    GetCameraSubsystem,
    CleanupCameraProperties,
    RemoveClump,
    ResetClumpMaterials,
    CleanupGeometry,
    GetMovementAdapter,
    CleanupMovementComponent,
    CleanupCache,
    CleanupConfig,
    ResetObjectList,
    CleanupAuxObjectFirst,
    CleanupAuxObjectSecond,
    GetActorControl,
    ClearActor,
    GetPlayerManager,
    ForAllPlayers,
    UpdatePlayers,
    RemovePetScript,
    GetPetManager,
    CleanupPets,
    GetNpcManager,
    ForAllNpcs,
    CleanupNpcs,
    GetMonsterManager,
    ForAllMonsters,
    CleanupMonsters,
    GetObjectManager,
    ForAllObjects,
    CleanupObjects,
    GetSecondaryObjectManager,
    ForAllSecondaryObjects,
    CleanupSecondaryObjects,
    GetPathPlanner,
    CleanupPaths,
    GetGameController,
    CleanupWorld,
    DestroyPipeline,
};

class IGameArenaReleaseRuntime : public IGameArenaRecordMemory {
public:
    virtual ~IGameArenaReleaseRuntime() = default;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    virtual void StoreText(std::uint32_t address,
                           std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaReleaseCall operation, std::uint32_t first = 0u,
        std::uint32_t second = 0u, std::uint32_t third = 0u) = 0;
};

class GameArenaReleaseRuntimeStub final : public IGameArenaReleaseRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::string_view ReadCString(std::uint32_t) override {
        return {};
    }
    void StoreText(std::uint32_t, std::string_view) override {}
    [[nodiscard]] std::uint32_t Call(GameArenaReleaseCall,
                                      std::uint32_t, std::uint32_t,
                                      std::uint32_t) override { return 0u; }
};

class GameArenaRelease final {
public:
    // IDA51B29A..51B36E, after script cleanup and the loaded-world gate.
    // Keep effects, borrowed records and skill cleanup in the original order.
    static void ReleaseEffectsAndRecords(std::uint32_t arena,
                                         IGameArenaReleaseRuntime& runtime);
    // IDA51B3DC..51B440. Retire render borrowers before scene object release.
    static void ReleaseRenderBindings(std::uint32_t arena,
                                      IGameArenaReleaseRuntime& runtime);
    // IDA51B448..51B472: movement components, sequence resources, dual sky.
    static void ReleaseMovementAndSceneResources(std::uint32_t arena,
                                                 IGameArenaReleaseRuntime& runtime);
    // IDA51B4F0..51B569: scene leave then cleanup for each manager, then paths.
    static void ReleaseObjectManagers(std::uint32_t arena,
                                      IGameArenaReleaseRuntime& runtime);
    // IDA51B56E..51B59A: world cleanup, Scene-record clear, optional pipeline.
    static void ReleaseWorldAndPipeline(std::uint32_t arena,
                                        IGameArenaReleaseRuntime& runtime);
    // IDA 0x51b270.
    [[nodiscard]] static bool Release(std::uint32_t arena,
                                      IGameArenaReleaseRuntime& runtime);
};

}  // namespace pal4
