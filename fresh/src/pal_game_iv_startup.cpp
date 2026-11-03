#include "pal4/pal_game_iv_startup.h"

#include <string>

namespace pal4 {

void PalGameIVStartup::Prepare(
    std::uint32_t game, IPalGameIVStartupPreparationRuntime& runtime) {
    runtime.InitializeInventory(runtime.Inventory());
    runtime.ProcessLoadedPlayers(runtime.PlayerManager());
    runtime.ClearScriptCache(runtime.ScriptInstance() + kScriptCacheOffset);
    runtime.ConfigureCamera(runtime.Camera());
    runtime.ClearArenaCache(runtime.ArenaCache());
    runtime.InitializeCoreSystems(game);
    runtime.SaveDefaultDialogSettings(runtime.GlobalData());
    runtime.WriteWord(runtime.PetManager() + kPetStateOffset, 0u);

}

std::uint32_t PalGameIVStartup::Initialize(
    std::uint32_t game, IPalGameIVStartupRuntime& runtime) {
    Prepare(game, runtime);

    const bool csb_mode = runtime.IsCsbMode();
    const std::string source_path = csb_mode
        ? "gamedata\\editData\\script\\script.csb"
        : "gamedata\\editData\\script\\script.cs";
    runtime.InitializeScriptEngine();
    const std::string script_name = "PAL4-NewGame";
    const std::string full_name = script_name + "[NewGame]";
    const auto cache = runtime.ScriptInstance() + kScriptCacheOffset;
    if (!runtime.ScriptCacheContains(cache, full_name)) {
        const auto interpreter = runtime.CreateInterpreter(
            csb_mode, script_name, full_name, "NewGame", source_path,
            kReadToBufferCallback);
        if (interpreter != 0u && runtime.AddScriptModule(
                runtime.ScriptInstance() + kScriptCacheOffset, interpreter)) {
            const auto script_cache =
                runtime.ScriptInstance() + kScriptCacheOffset;
            const std::string name(runtime.InterpreterName(interpreter));
            const std::string full_path(
                runtime.InterpreterFullPath(interpreter));
            runtime.PushStackModule(script_cache, full_path, name);
            runtime.ReleaseInterpreter(interpreter);
        } else {
            runtime.ShowMissingScriptMessage("not found script files",
                                             "no script");
        }
    }
    runtime.CleanupAllScripts();
    return 1u;
}

}  // namespace pal4
