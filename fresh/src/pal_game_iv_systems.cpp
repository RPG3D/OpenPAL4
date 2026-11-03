#include "pal4/pal_game_iv_systems.h"

namespace pal4 {
namespace {

std::uint32_t Call(IPalGameIVSystemsRuntime& runtime,
                   PalGameIVSystemStep step, std::uint32_t context = 0u,
                   std::uint32_t argument = 0u,
                   std::uint32_t auxiliary = 0u,
                   std::string_view resource = {}) {
    return runtime.Invoke({step, context, argument, auxiliary, resource});
}

}  // namespace

std::uint32_t PalGameIVSystems::Initialize(
    std::uint32_t object, std::uint32_t effect_context,
    IPalGameIVSystemsRuntime& runtime) {
    using S = PalGameIVSystemStep;
    (void)Call(runtime, S::FloatOperation);
    (void)Call(runtime, S::CalculateSquareRootTable);
    if (runtime.ReadGameStateWord(0x58cu) != 0u) {
        const auto pk = Call(runtime, S::PkFileManagerInstance);
        (void)Call(runtime, S::PkFileManagerLoadAllScripts, pk);
    }
    (void)Call(runtime, S::InitializeLightsAndHierarchy);
    auto instance = Call(runtime, S::LoadingFrameInstance);
    (void)Call(runtime, S::LoadingFrameUpdateAndRender, instance);
    instance = Call(runtime, S::GameDatabaseInstance);
    (void)Call(runtime, S::GameDatabaseInitialize, instance, 0u, 0u,
               "gamedata\\database\\pal4db.db");
    instance = Call(runtime, S::InventoryInstance);
    (void)Call(runtime, S::InventoryInitialize, instance);
    instance = Call(runtime, S::ScriptInstance);
    (void)Call(runtime, S::ScriptObjectInitialize, instance + 0x54u);
    instance = Call(runtime, S::CameraInstance);
    (void)Call(runtime, S::CameraInitialize, instance);
    instance = Call(runtime, S::TextureDictionaryInstance);
    (void)Call(runtime, S::TextureDictionaryInitialize, instance);
    instance = Call(runtime, S::FontManagerInstance);
    (void)Call(runtime, S::FontManagerInitialize, instance);
    instance = Call(runtime, S::ScreenEffectInstance);
    (void)Call(runtime, S::ScreenEffectInitialize, instance);
    (void)Call(runtime, S::PkFileManagerProcessScript7);
    instance = Call(runtime, S::EffectSystemInstance);
    (void)Call(runtime, S::EffectSystemInitialize, instance, 1u,
               effect_context);
    (void)Call(runtime, S::PkFileManagerCleanupScripts);
    instance = Call(runtime, S::ArenaEffectInstance);
    (void)Call(runtime, S::ArenaEffectInitialize, instance);
    (void)Call(runtime, S::PkFileManagerProcessScript8);
    (void)Call(runtime, S::ExecuteEffectScript, 0u, 0u, 0u,
               "gamedata\\MatFX\\Shader\\ltMap.fx");
    (void)Call(runtime, S::PkFileManagerCleanupScripts);
    instance = Call(runtime, S::ClumpManagerInstance);
    (void)Call(runtime, S::ClumpManagerLoad, instance, 0u, 0u,
               "gamedata\\Common\\rpHookNameTable.hnt");
    (void)Call(runtime, S::ActorMessageInstance);
    instance = Call(runtime, S::PlayerManagerInstance);
    (void)Call(runtime, S::PlayerManagerLoad, instance);
    instance = Call(runtime, S::ActorControlInstance);
    (void)Call(runtime, S::ActorControlInitialize, instance);
    instance = Call(runtime, S::UiFrameManagerInstance);
    (void)Call(runtime, S::InitializeCegui, instance);
    instance = Call(runtime, S::CombatSystemInstance);
    (void)Call(runtime, S::CombatSystemInitialize, instance);
    instance = Call(runtime, S::AudioManagerInstance);
    (void)Call(runtime, S::AudioSystemInitialize, instance);
    (void)Call(runtime, S::AtomicPluginFloat, 0u, 0x44960000u);
    (void)Call(runtime, S::InitializeComponents, object);
    (void)Call(runtime, S::RegisterGameObjects, object);
    (void)Call(runtime, S::InitializeCoreSystems, object);
    instance = Call(runtime, S::GlobalDataInstance);
    (void)Call(runtime, S::DialogManagerLoadSettings, instance);
    instance = Call(runtime, S::UiFrameManagerInstance);
    (void)Call(runtime, S::RegisterUiWindows, instance);
    const auto result = Call(runtime, S::InitializeConsoleCommands);
    runtime.WriteWord(object + 0x174u, 1u);
    return result;
}

std::uint32_t PalGameIVSystems::Shutdown(
    std::uint32_t object, std::uint32_t effect_context,
    IPalGameIVSystemsRuntime& runtime) {
    using S = PalGameIVSystemStep;
    auto instance = Call(runtime, S::GlobalDataInstance);
    (void)Call(runtime, S::DialogManagerSaveSettings, instance);
    instance = Call(runtime, S::BinkPlayerInstance);
    (void)Call(runtime, S::BinkPlayerCleanup, instance);
    (void)Call(runtime, S::GameStateCleanup, object);
    (void)Call(runtime, S::DestroyLightmapPipelines);
    (void)Call(runtime, S::TextureObjectDestruct, object + 0x178u);
    instance = Call(runtime, S::AudioManagerInstance);
    (void)Call(runtime, S::AudioSystemShutdown, instance);
    instance = Call(runtime, S::MinimapManagerInstance);
    (void)Call(runtime, S::MinimapReset, instance);
    instance = Call(runtime, S::GameArenaInstance);
    (void)Call(runtime, S::GameArenaRelease, instance);
    instance = Call(runtime, S::GameArenaInstance);
    (void)Call(runtime, S::GameArenaCleanupObjects, instance);
    instance = Call(runtime, S::ArenaCacheInstance);
    (void)Call(runtime, S::ArenaCacheClear, instance);
    instance = Call(runtime, S::CombatSystemInstance);
    (void)Call(runtime, S::CombatSystemShutdown, instance);
    instance = Call(runtime, S::ActorControlInstance);
    (void)Call(runtime, S::ActorControlShutdown, instance);
    instance = Call(runtime, S::ScriptInstance);
    (void)Call(runtime, S::ScriptObjectCleanup, instance + 0x54u);
    instance = Call(runtime, S::MovementAdapterInstance);
    (void)Call(runtime, S::MovementAdapterDestruct, instance);
    instance = Call(runtime, S::PlayerManagerInstance);
    (void)Call(runtime, S::PlayerManagerReset, instance);
    instance = Call(runtime, S::ScreenEffectInstance);
    (void)Call(runtime, S::ScreenEffectRelease, instance);
    instance = Call(runtime, S::ArenaEffectInstance);
    (void)Call(runtime, S::ArenaEffectShutdown, instance);
    instance = Call(runtime, S::UiFrameManagerInstance);
    (void)Call(runtime, S::UiFrameManagerCleanup, instance);
    instance = Call(runtime, S::EffectSystemInstance);
    (void)Call(runtime, S::EffectSystemShutdown, instance, effect_context);
    (void)Call(runtime, S::EffectScriptVmRelease);
    instance = Call(runtime, S::ClumpManagerInstance);
    (void)Call(runtime, S::ClumpManagerCleanup, instance);
    instance = Call(runtime, S::ActorMessageInstance);
    (void)Call(runtime, S::ActorMessageClear, instance);
    instance = Call(runtime, S::FontManagerInstance);
    (void)Call(runtime, S::FontManagerRelease, instance);
    instance = Call(runtime, S::TextureDictionaryInstance);
    (void)Call(runtime, S::TextureDictionaryCleanup, instance);
    instance = Call(runtime, S::CameraInstance);
    (void)Call(runtime, S::CameraCleanup, instance);
    instance = Call(runtime, S::GameDatabaseInstance);
    (void)Call(runtime, S::GameDatabaseCleanup, instance);
    instance = Call(runtime, S::LoadingFrameInstance);
    (void)Call(runtime, S::LoadingFrameGetProperty, instance);
    instance = Call(runtime, S::PkFileManagerInstance);
    (void)Call(runtime, S::PkFileManagerCleanupAndProcess, instance);
    auto result = Call(runtime, S::ObjectTextureDictionaryCleanup,
                       object + 0x1acu);
    const auto begin = runtime.ReadWord(object + 0x1a0u);
    const auto end = runtime.ReadWord(object + 0x1a4u);
    for (auto cursor = begin; cursor != end; cursor += 12u)
        result = Call(runtime, S::StringCleanup, cursor);
    runtime.WriteWord(object + 0x1a4u, begin);
    runtime.WriteWord(object + 0x174u, 0u);
    return result;
}

}  // namespace pal4
