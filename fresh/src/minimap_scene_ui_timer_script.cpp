#include "pal4/minimap_scene_ui_timer_script.h"

namespace pal4 {

std::uint32_t MinimapSceneUiTimerScript::Execute(
    MinimapSceneUiState& state, const std::uint32_t owner,
    IMinimapSceneUiTimerScriptRuntime& runtime) {
    const auto countdown = state.ReadWord(0xf0u);
    const auto countdown_text =
        std::to_string(static_cast<std::int32_t>(countdown));
    try {
        runtime.SetWindowText(state.ReadWord(0xbcu), countdown_text);
        const auto remaining = countdown - 1u;
        state.WriteWord(0xf0u, remaining);
        if (static_cast<std::int32_t>(remaining) < 0) {
            runtime.SetWindowText(state.ReadWord(0xbcu),
                                  runtime.EmptyErrorText());
            const auto queue = runtime.ReadWord(runtime.FrameManager() + 0x1a0u);
            runtime.RemoveTimer(queue, owner + 0xd0u);
            runtime.InitializeScriptSystem();

            const auto context =
                runtime.CopyContext(runtime.ArenaManager() + 0xd4u);
            const std::string script_path{kScriptPath};
            const auto full_path = script_path + std::string{kScriptSuffix};
            auto engine = runtime.ScriptEngine();
            if (!runtime.HasStackModule(engine + 0x54u, full_path)) {
                auto script_name = state.ReadWord(0xe4u);
                if (script_name == 0u) {
                    script_name = runtime.EmptyLogMessage();
                }
                const auto interpreter = runtime.CreateInterpreter(
                    runtime.ScriptMode(), script_path, full_path,
                    script_name, context, kReaderFunction);
                if (interpreter != 0u) {
                    engine = runtime.ScriptEngine();
                    if (runtime.AddScriptModule(
                            engine + 0x54u, interpreter)) {
                        engine = runtime.ScriptEngine();
                        const auto function_name =
                            runtime.FunctionName(interpreter);
                        const auto full_path =
                            runtime.FullPath(interpreter);
                        runtime.PushStackModule(engine + 0x54u,
                                                full_path, function_name);
                        runtime.ReleaseInterpreter(interpreter);
                    }
                }
            }
            runtime.CleanupAllScripts();
        }
    } catch (...) {
        (void)runtime.CleanupCountdown(countdown_text);
        throw;
    }
    return runtime.CleanupCountdown(countdown_text);
}

}  // namespace pal4
