#include "pal4/property_window_script_action.h"

namespace pal4 {

std::uint32_t PropertyWindowScriptAction::ExecuteEarthballSkip(
    IPropertyWindowScriptRuntime& runtime) {
    runtime.PlayMusic("WA43.wav", 0u);
    StringBuffer context;
    static_cast<void>(context.CopyConstruct(
        StringBuffer(runtime.ReadStringState(runtime.ArenaManager() + 0xd4u)),
        runtime));

    StringBuffer base_path;
    StringBuffer full_path;
    bool base_initialized = false;
    bool full_initialized = false;
    try {
        runtime.InitializeScripts();
        base_initialized = true;
        static_cast<void>(base_path.InitializeFromCString(
            runtime.LiteralAddress(EarthballLiteral::ScriptPath), runtime));
        full_initialized = true;
        static_cast<void>(full_path.CopyConstruct(base_path, runtime));
        const auto suffix = runtime.LiteralAddress(EarthballLiteral::PathSuffix);
        static_cast<void>(full_path.Append(suffix, 11u, runtime));

        if (!runtime.HasStackModule(runtime.ScriptEngine() + 0x54u,
                                   full_path.State().data)) {
            const auto interpreter = runtime.CreateInterpreter(
                runtime.ScriptMode(), base_path.State().data,
                full_path.State().data, "EarthBall", context.State().data,
                kReadToBufferCallback);
            if (interpreter != 0u &&
                runtime.AddScriptModule(runtime.ScriptEngine() + 0x54u,
                                        interpreter)) {
                const auto cache = runtime.ScriptEngine() + 0x54u;
                const auto name = runtime.FunctionName(interpreter);
                const auto path = runtime.FullPath(interpreter);
                runtime.PushStackModule(cache, path, name);
                runtime.ReleaseInterpreter(interpreter);
            }
        }
        runtime.CleanupAllScripts();
        runtime.ProcessEvents(runtime.UIFrameManager() + 0x104u,
                              kEventId, {0u, 0u});
    } catch (...) {
        if (full_initialized) static_cast<void>(full_path.Cleanup(runtime));
        if (base_initialized) static_cast<void>(base_path.Cleanup(runtime));
        static_cast<void>(context.Cleanup(runtime));
        throw;
    }

    static_cast<void>(full_path.Cleanup(runtime));
    static_cast<void>(base_path.Cleanup(runtime));
    return context.Cleanup(runtime);
}

}  // namespace pal4
