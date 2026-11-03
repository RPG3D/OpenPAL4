#include "pal4/pal_game_iv_systems.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

using S = pal4::PalGameIVSystemStep;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint32_t Handle(S step) {
    return 0x4000u + static_cast<std::uint32_t>(step) * 0x100u;
}

struct Runtime final : pal4::IPalGameIVSystemsRuntime {
    std::vector<pal4::PalGameIVSystemCall> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t game_state = 0u;
    std::uint32_t game_state_reads = 0u;
    std::uint32_t last_flag_write_after = 0u;

    std::uint32_t Invoke(const pal4::PalGameIVSystemCall& call) override {
        calls.push_back(call);
        if (call.step == S::InitializeConsoleCommands) return 0xcafeu;
        if (call.step == S::ObjectTextureDictionaryCleanup) return 0xbeefu;
        if (call.step == S::StringCleanup) return call.context + 1u;
        return Handle(call.step);
    }
    std::uint32_t ReadGameStateWord(std::uint32_t offset) override {
        Check(offset == 0x58cu, "game state offset");
        ++game_state_reads;
        return game_state;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        if (address == 0x1174u) last_flag_write_after =
            static_cast<std::uint32_t>(calls.size());
    }
};

std::vector<S> StartupSteps(bool preload) {
    std::vector<S> steps = {S::FloatOperation, S::CalculateSquareRootTable};
    if (preload) {
        steps.push_back(S::PkFileManagerInstance);
        steps.push_back(S::PkFileManagerLoadAllScripts);
    }
    const std::vector<S> rest = {
        S::InitializeLightsAndHierarchy, S::LoadingFrameInstance,
        S::LoadingFrameUpdateAndRender, S::GameDatabaseInstance,
        S::GameDatabaseInitialize, S::InventoryInstance,
        S::InventoryInitialize, S::ScriptInstance,
        S::ScriptObjectInitialize, S::CameraInstance,
        S::CameraInitialize, S::TextureDictionaryInstance,
        S::TextureDictionaryInitialize, S::FontManagerInstance,
        S::FontManagerInitialize, S::ScreenEffectInstance,
        S::ScreenEffectInitialize, S::PkFileManagerProcessScript7,
        S::EffectSystemInstance, S::EffectSystemInitialize,
        S::PkFileManagerCleanupScripts, S::ArenaEffectInstance,
        S::ArenaEffectInitialize, S::PkFileManagerProcessScript8,
        S::ExecuteEffectScript, S::PkFileManagerCleanupScripts,
        S::ClumpManagerInstance, S::ClumpManagerLoad,
        S::ActorMessageInstance, S::PlayerManagerInstance,
        S::PlayerManagerLoad, S::ActorControlInstance,
        S::ActorControlInitialize, S::UiFrameManagerInstance,
        S::InitializeCegui, S::CombatSystemInstance,
        S::CombatSystemInitialize, S::AudioManagerInstance,
        S::AudioSystemInitialize, S::AtomicPluginFloat,
        S::InitializeComponents, S::RegisterGameObjects,
        S::InitializeCoreSystems, S::GlobalDataInstance,
        S::DialogManagerLoadSettings, S::UiFrameManagerInstance,
        S::RegisterUiWindows, S::InitializeConsoleCommands,
    };
    steps.insert(steps.end(), rest.begin(), rest.end());
    return steps;
}

void CheckSteps(const Runtime& runtime, const std::vector<S>& expected) {
    Check(runtime.calls.size() == expected.size(), "step count");
    for (std::size_t index = 0; index < expected.size(); ++index)
        Check(runtime.calls[index].step == expected[index], "step order");
}

const pal4::PalGameIVSystemCall& Find(const Runtime& runtime, S step,
                                      std::size_t occurrence = 0u) {
    for (const auto& call : runtime.calls) {
        if (call.step != step) continue;
        if (occurrence-- == 0u) return call;
    }
    throw std::runtime_error("missing step");
}

void Initialize(bool preload) {
    Runtime runtime;
    runtime.game_state = preload ? 1u : 0u;
    Check(pal4::PalGameIVSystems::Initialize(0x1000u, 0x3456u, runtime) ==
              0xcafeu, "console result");
    CheckSteps(runtime, StartupSteps(preload));
    Check(runtime.game_state_reads == 1u &&
              runtime.ReadWord(0x1174u) == 1u &&
              runtime.last_flag_write_after == runtime.calls.size(),
          "ready flag timing");
    if (preload)
        Check(Find(runtime, S::PkFileManagerLoadAllScripts).context ==
                  Handle(S::PkFileManagerInstance), "conditional PK handle");
    Check(Find(runtime, S::LoadingFrameUpdateAndRender).context ==
              Handle(S::LoadingFrameInstance), "loading frame handle");
    Check(Find(runtime, S::GameDatabaseInitialize).resource ==
              "gamedata\\database\\pal4db.db", "database path");
    Check(Find(runtime, S::ScriptObjectInitialize).context ==
              Handle(S::ScriptInstance) + 0x54u, "script object offset");
    const auto effect = Find(runtime, S::EffectSystemInitialize);
    Check(effect.context == Handle(S::EffectSystemInstance) &&
              effect.argument == 1u && effect.auxiliary == 0x3456u,
          "effect setup arguments");
    Check(Find(runtime, S::ExecuteEffectScript).resource ==
              "gamedata\\MatFX\\Shader\\ltMap.fx", "effect path");
    Check(Find(runtime, S::ClumpManagerLoad).resource ==
              "gamedata\\Common\\rpHookNameTable.hnt", "clump path");
    Check(Find(runtime, S::AtomicPluginFloat).argument == 0x44960000u,
          "atomic float bits");
    for (auto step : {S::InitializeComponents, S::RegisterGameObjects,
                      S::InitializeCoreSystems})
        Check(Find(runtime, step).context == 0x1000u,
              "game object forwarding");
    Check(Find(runtime, S::RegisterUiWindows).context ==
              Handle(S::UiFrameManagerInstance), "UI manager handle");
}

std::vector<S> ShutdownSteps(bool has_strings) {
    std::vector<S> steps = {
        S::GlobalDataInstance, S::DialogManagerSaveSettings,
        S::BinkPlayerInstance, S::BinkPlayerCleanup, S::GameStateCleanup,
        S::DestroyLightmapPipelines, S::TextureObjectDestruct,
        S::AudioManagerInstance, S::AudioSystemShutdown,
        S::MinimapManagerInstance, S::MinimapReset, S::GameArenaInstance,
        S::GameArenaRelease, S::GameArenaInstance,
        S::GameArenaCleanupObjects, S::ArenaCacheInstance,
        S::ArenaCacheClear, S::CombatSystemInstance,
        S::CombatSystemShutdown, S::ActorControlInstance,
        S::ActorControlShutdown, S::ScriptInstance,
        S::ScriptObjectCleanup, S::MovementAdapterInstance,
        S::MovementAdapterDestruct, S::PlayerManagerInstance,
        S::PlayerManagerReset, S::ScreenEffectInstance,
        S::ScreenEffectRelease, S::ArenaEffectInstance,
        S::ArenaEffectShutdown, S::UiFrameManagerInstance,
        S::UiFrameManagerCleanup, S::EffectSystemInstance,
        S::EffectSystemShutdown, S::EffectScriptVmRelease,
        S::ClumpManagerInstance, S::ClumpManagerCleanup,
        S::ActorMessageInstance, S::ActorMessageClear,
        S::FontManagerInstance, S::FontManagerRelease,
        S::TextureDictionaryInstance, S::TextureDictionaryCleanup,
        S::CameraInstance, S::CameraCleanup, S::GameDatabaseInstance,
        S::GameDatabaseCleanup, S::LoadingFrameInstance,
        S::LoadingFrameGetProperty, S::PkFileManagerInstance,
        S::PkFileManagerCleanupAndProcess,
        S::ObjectTextureDictionaryCleanup,
    };
    if (has_strings) {
        steps.push_back(S::StringCleanup);
        steps.push_back(S::StringCleanup);
    }
    return steps;
}

void Shutdown(bool has_strings) {
    Runtime runtime;
    runtime.WriteWord(0x11a0u, has_strings ? 0x7000u : 0u);
    runtime.WriteWord(0x11a4u, has_strings ? 0x7018u : 0u);
    runtime.calls.clear();
    const auto result = pal4::PalGameIVSystems::Shutdown(
        0x1000u, 0x4567u, runtime);
    Check(result == (has_strings ? 0x700du : 0xbeefu), "shutdown result");
    CheckSteps(runtime, ShutdownSteps(has_strings));
    Check(runtime.ReadWord(0x11a4u) == runtime.ReadWord(0x11a0u) &&
              runtime.ReadWord(0x1174u) == 0u &&
              runtime.last_flag_write_after == runtime.calls.size(),
          "shutdown flags");
    Check(Find(runtime, S::GameStateCleanup).context == 0x1000u &&
              Find(runtime, S::TextureObjectDestruct).context == 0x1178u &&
              Find(runtime, S::ObjectTextureDictionaryCleanup).context ==
                  0x11acu, "native object offsets");
    Check(Find(runtime, S::ScriptObjectCleanup).context ==
              Handle(S::ScriptInstance) + 0x54u, "script cleanup offset");
    const auto effect = Find(runtime, S::EffectSystemShutdown);
    Check(effect.context == Handle(S::EffectSystemInstance) &&
              effect.argument == 0x4567u, "effect shutdown argument");
    if (has_strings) {
        Check(Find(runtime, S::StringCleanup).context == 0x7000u &&
                  Find(runtime, S::StringCleanup, 1u).context == 0x700cu,
              "string collection traversal");
    }
}

}  // namespace

int main() {
    try {
        Initialize(false);
        Initialize(true);
        Shutdown(false);
        Shutdown(true);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
