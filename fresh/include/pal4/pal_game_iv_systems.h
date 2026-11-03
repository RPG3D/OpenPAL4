#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class PalGameIVSystemStep {
    FloatOperation,
    CalculateSquareRootTable,
    PkFileManagerInstance,
    PkFileManagerLoadAllScripts,
    InitializeLightsAndHierarchy,
    LoadingFrameInstance,
    LoadingFrameUpdateAndRender,
    GameDatabaseInstance,
    GameDatabaseInitialize,
    InventoryInstance,
    InventoryInitialize,
    ScriptInstance,
    ScriptObjectInitialize,
    CameraInstance,
    CameraInitialize,
    TextureDictionaryInstance,
    TextureDictionaryInitialize,
    FontManagerInstance,
    FontManagerInitialize,
    ScreenEffectInstance,
    ScreenEffectInitialize,
    PkFileManagerProcessScript7,
    EffectSystemInstance,
    EffectSystemInitialize,
    PkFileManagerCleanupScripts,
    ArenaEffectInstance,
    ArenaEffectInitialize,
    PkFileManagerProcessScript8,
    ExecuteEffectScript,
    ClumpManagerInstance,
    ClumpManagerLoad,
    ActorMessageInstance,
    PlayerManagerInstance,
    PlayerManagerLoad,
    ActorControlInstance,
    ActorControlInitialize,
    UiFrameManagerInstance,
    InitializeCegui,
    CombatSystemInstance,
    CombatSystemInitialize,
    AudioManagerInstance,
    AudioSystemInitialize,
    AtomicPluginFloat,
    InitializeComponents,
    RegisterGameObjects,
    InitializeCoreSystems,
    GlobalDataInstance,
    DialogManagerLoadSettings,
    RegisterUiWindows,
    InitializeConsoleCommands,
    DialogManagerSaveSettings,
    BinkPlayerInstance,
    BinkPlayerCleanup,
    GameStateCleanup,
    DestroyLightmapPipelines,
    TextureObjectDestruct,
    AudioSystemShutdown,
    MinimapManagerInstance,
    MinimapReset,
    GameArenaInstance,
    GameArenaRelease,
    GameArenaCleanupObjects,
    ArenaCacheInstance,
    ArenaCacheClear,
    CombatSystemShutdown,
    ActorControlShutdown,
    ScriptObjectCleanup,
    MovementAdapterInstance,
    MovementAdapterDestruct,
    PlayerManagerReset,
    ScreenEffectRelease,
    ArenaEffectShutdown,
    UiFrameManagerCleanup,
    EffectSystemShutdown,
    EffectScriptVmRelease,
    ClumpManagerCleanup,
    ActorMessageClear,
    FontManagerRelease,
    TextureDictionaryCleanup,
    CameraCleanup,
    GameDatabaseCleanup,
    LoadingFrameGetProperty,
    PkFileManagerCleanupAndProcess,
    ObjectTextureDictionaryCleanup,
    StringCleanup,
};

struct PalGameIVSystemCall {
    PalGameIVSystemStep step;
    std::uint32_t context = 0;
    std::uint32_t argument = 0;
    std::uint32_t auxiliary = 0;
    std::string_view resource;
};

class IPalGameIVSystemsRuntime {
public:
    virtual ~IPalGameIVSystemsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Invoke(
        const PalGameIVSystemCall& call) = 0;
    [[nodiscard]] virtual std::uint32_t ReadGameStateWord(
        std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class PalGameIVSystems final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t effect_context,
        IPalGameIVSystemsRuntime& runtime);
    [[nodiscard]] static std::uint32_t Shutdown(
        std::uint32_t object, std::uint32_t effect_context,
        IPalGameIVSystemsRuntime& runtime);
};

}  // namespace pal4
