#include "pal4/pal_game_object_factory_script.h"

namespace pal4 {
namespace {

std::uint32_t Execute(
    const std::uint32_t handle, const std::uint32_t argument,
    const bool require_name, const bool set_movement_flag,
    IPalGameObjectFactoryScriptRuntime& runtime) {
    if (argument != 0u) return 0u;
    const auto object = runtime.ReadWord(handle + 4u);
    if (object == 0u) return 0u;
    const auto name_length = runtime.ReadWord(object + 52u);
    if (require_name && static_cast<std::int32_t>(name_length) <= 0)
        return 0u;

    runtime.InitializeScript();
    const auto arena = runtime.ArenaManager();
    const auto path = runtime.ReadCString(runtime.ReadWord(arena + 200u));
    const auto function = runtime.ReadBytes(
        runtime.ReadWord(object + 48u), name_length);
    const auto key = path + "[" + function + "]";
    std::uint32_t result = 0u;
    if (!runtime.ScriptCacheContains(key)) {
        const auto interpreter = runtime.CreateScriptInterpreter(
            runtime.IsCsbMode(), path, key, function,
            runtime.ReadWord(arena + 212u));
        if (interpreter != 0u &&
            runtime.AddScriptModule(interpreter)) {
            const auto name = runtime.ScriptFunctionName(interpreter);
            const auto full_path = runtime.ScriptFullPath(interpreter);
            if (runtime.PushStackModule(full_path, name)==0) {
                result = 1u;
                if (set_movement_flag) {
                    const auto adapter = runtime.MovementAdapter();
                    runtime.WriteWord(runtime.ReadWord(adapter) + 88u, 1u);
                }
            }
            runtime.ReleaseInterpreter(interpreter);
        } else {
            runtime.ShowMissingScriptMessage();
        }
    }
    runtime.CleanupAllScripts();
    return result;
}

}  // namespace

std::uint32_t PalGameObjectFactoryScript::ExecuteLocalFloor(
    const std::uint32_t handle, const std::uint32_t argument,
    IPalGameObjectFactoryScriptRuntime& runtime) {
    return Execute(handle, argument, false, true, runtime);
}

std::uint32_t PalGameObjectFactoryScript::ExecuteMachine(
    const std::uint32_t handle, const std::uint32_t argument,
    IPalGameObjectFactoryScriptRuntime& runtime) {
    return Execute(handle, argument, true, false, runtime);
}

}  // namespace pal4
