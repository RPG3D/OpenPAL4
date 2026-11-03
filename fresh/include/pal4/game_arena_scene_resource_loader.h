#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class ArenaScenePathRoot : std::uint8_t {
    WorldPrefix,
    WorldSuffix,
    MovementPrefix,
};

enum class ArenaSceneResourceCall : std::uint8_t {
    InitializeTemperature,
    SetTemperatureState,
    InitializeArenaCache,
    InitializeArenaModule,
    InitializeCamera,
    ConfigureCamera,
    LoadArchive,
    ProcessArchive,
    CleanupScripts,
    DestroyArchive,
    GetActiveCamera,
    SetWorldCamera,
    LoadLightmapConfig,
    DestroySkinPipeline,
    CreateSkinPipeline,
    ApplySkinPipeline,
    DestroyFogDefaults,
    FreeMemory,
    AllocateFogDefaults,
    InitializeFogDefaults,
    LoadFogConfig,
    InitializePluginData,
    AddClump,
    ProcessClumpAtomics,
    ProcessClump,
    GetMovementAdapter,
    LoadIntriClump,
    LoadSequenceXml,
    ApplySequenceEntries,
    ProcessScriptStage,
    LoadFloorClump,
    LoadWallClump,
    RegisterEffectSingleton,
};

class IGameArenaSceneResourceRuntime {
public:
    virtual ~IGameArenaSceneResourceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view PathRoot(ArenaScenePathRoot root) = 0;
    [[nodiscard]] virtual bool ArchiveModeEnabled() = 0;
    [[nodiscard]] virtual std::uint32_t GameContext() = 0;
    virtual void RemoveArenaCache(std::string_view world_name,
                                  std::string_view cache_name) = 0;
    virtual void UpdateProgress(float value) = 0;
    [[nodiscard]] virtual bool InitializeWorld(
        std::uint32_t object, std::string_view world_root,
        std::string_view scene, std::string_view section,
        std::uint32_t context) = 0;
    virtual void AddAmbientLight(std::uint32_t object,
                                 float red, float green, float blue,
                                 float alpha) = 0;
    virtual void LoadSkyAssets(std::uint32_t config,
                               std::string_view path,
                               std::string_view sky,
                               std::string_view cloud,
                               std::string_view sky_dual,
                               std::string_view cloud_dual,
                               std::uint32_t first_index,
                               std::uint32_t second_index) = 0;
    virtual void InitializeEffects(std::uint32_t singleton,
                                   std::uint32_t scene_effect,
                                   std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        ArenaSceneResourceCall operation, std::uint32_t first = 0u,
        std::uint32_t second = 0u,
        std::string_view text = {}) = 0;
};

class GameArenaSceneResourceRuntimeStub final
    : public IGameArenaSceneResourceRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::string_view ReadText(std::uint32_t) override { return {}; }
    [[nodiscard]] std::string_view PathRoot(ArenaScenePathRoot) override {
        return {};
    }
    [[nodiscard]] bool ArchiveModeEnabled() override { return false; }
    [[nodiscard]] std::uint32_t GameContext() override { return 0u; }
    void RemoveArenaCache(std::string_view, std::string_view) override {}
    void UpdateProgress(float) override {}
    [[nodiscard]] bool InitializeWorld(
        std::uint32_t, std::string_view, std::string_view,
        std::string_view, std::uint32_t) override { return false; }
    void AddAmbientLight(std::uint32_t, float, float, float, float) override {}
    void LoadSkyAssets(std::uint32_t, std::string_view, std::string_view,
                       std::string_view, std::string_view, std::string_view,
                       std::uint32_t, std::uint32_t) override {}
    void InitializeEffects(std::uint32_t, std::uint32_t,
                           std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Call(
        ArenaSceneResourceCall, std::uint32_t, std::uint32_t,
        std::string_view) override { return 0u; }
};

class GameArenaSceneResourceLoader final {
public:
    // IDA 0x518a40.
    [[nodiscard]] static bool Load(std::uint32_t object,
                                   std::string_view scene,
                                   std::string_view section,
                                   std::uint32_t reuse_flag,
                                   IGameArenaSceneResourceRuntime& runtime);
};

}  // namespace pal4
