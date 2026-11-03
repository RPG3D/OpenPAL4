#include "pal4/game_world_scene_script_loader.h"

#include <algorithm>

namespace pal4 {
namespace {

std::string CopyCString(const std::string_view text) {
    return std::string(text.substr(0, text.find('\0')));
}

std::string ReadSceneFragment(const std::uint32_t world,
                              IGameWorldSceneScriptNamesRuntime& runtime) {
    const auto text = runtime.ReadText(world + 224u);
    const auto length = runtime.ReadWord(world + 228u);
    return std::string(text.substr(0, std::min<std::size_t>(text.size(),
                                                             length)));
}

}  // namespace

GameWorldSceneScriptRequest GameWorldSceneScriptLoader::PrepareNames(
    const std::uint32_t world, IGameWorldSceneScriptNamesRuntime& runtime) {
    runtime.WriteWord(world + 236u, 0u);
    const std::string file_prefix(runtime.InitialFilePrefix());
    const auto world_name = CopyCString(runtime.ReadText(world + 8u));
    const auto file_mode = runtime.IsCsbMode();
    const std::string file_path =
        file_prefix + world_name +
        (file_mode ? ".csb" : ".cs");
    runtime.WriteStoredText(world + 212u, file_path);

    const std::string script_path = "PAL4-script:[" + world_name + "_" +
                                    ReadSceneFragment(world, runtime) + "]";
    runtime.WriteStoredText(world + 200u, script_path);
    const std::string function_name = world_name + "_" +
                                      ReadSceneFragment(world, runtime) +
                                      "_init";
    return {file_mode, script_path,
            script_path + "[" + CopyCString(function_name) + "]",
            function_name, file_path, kReadCallback};
}

std::uint32_t GameWorldSceneScriptLoader::Load(
    const std::uint32_t world, const std::uint32_t,
    IGameWorldSceneScriptRuntime& runtime) {
    auto request = PrepareNames(world, runtime);
    const auto& function_path = request.function_path;
    const auto& script_path = request.script_path;
    const auto& function_name = request.function_name;
    runtime.InitializeScriptSystem();
    if (!runtime.HasStackModule(runtime.GetExecutor(),
                               CopyCString(function_path))) {
        request.csb_mode = runtime.IsCsbMode();
        const auto interpreter = runtime.CreateInterpreter(request);
        if (interpreter != 0u &&
            runtime.AddScriptModule(runtime.GetExecutor(), interpreter)) {
            const auto executor = runtime.GetExecutor();
            const auto function = std::string(
                runtime.InterpreterFunctionName(interpreter));
            const auto full_path = std::string(
                runtime.InterpreterFullPath(interpreter));
            if (runtime.PushStackModule(executor, full_path, function) == 0)
                runtime.WriteWord(world + 236u, 1u);
            runtime.ReleaseInterpreter(interpreter);
        } else {
            runtime.ShowMissing("not found script files", "no script");
        }
    }

    runtime.CleanupScripts();
    static_cast<void>(runtime.ReleaseTemporary(
        GameWorldScriptTemporary::FunctionPath, function_path));
    static_cast<void>(runtime.ReleaseTemporary(
        GameWorldScriptTemporary::ScriptPath, script_path));
    return runtime.ReleaseTemporary(
        GameWorldScriptTemporary::FunctionName, function_name);
}

}  // namespace pal4
