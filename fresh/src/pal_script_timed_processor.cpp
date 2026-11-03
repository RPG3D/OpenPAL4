#include "pal4/pal_script_timed_processor.h"

#include <bit>

namespace pal4 {

std::uint32_t PalScriptTimedProcessor::Process(
    PalScriptExecutorState& state, const float delta,
    IPalScriptTimedRuntime& runtime) {
    if (state.words[15u] == 0u) return 0u;

    const auto elapsed = std::bit_cast<float>(state.words[16u]);
    const auto total = static_cast<double>(delta) + elapsed;
    state.words[16u] = std::bit_cast<std::uint32_t>(
        static_cast<float>(total));
    const auto delay = std::bit_cast<float>(state.words[17u]);
    if (!(total >= delay)) return 0x100u;

    state.words[16u] = 0u;
    if (!runtime.ActorControlActive()) {
        const auto executor = runtime.ExecutorAddress();
        if (runtime.ExecutorWord(executor + 96u) != 0u) return executor;
    }

    runtime.InitializeScriptSystem();
    StringBufferState script_path{};
    StringBufferState full_path{};
    bool path_ready = false;
    bool full_ready = false;
    try {
        runtime.InitializePath(script_path, "TimeScript");
        path_ready = true;
        runtime.CopyString(full_path, script_path);
        full_ready = true;
        runtime.AppendLiteral(full_path, "[", 1u);
        runtime.AppendText(full_path, state.words[18u], state.words[19u]);
        runtime.AppendLiteral(full_path, "]", 1u);

        const auto executor = runtime.ExecutorAddress();
        if (!runtime.HasStackModule(executor + 84u, full_path.data)) {
            TimedScriptInterpreterRequest request{};
            request.arena_context = runtime.ArenaContext();
            request.callback = state.words[18u];
            request.full_path = full_path.data;
            request.script_path = script_path.data;
            request.mode = runtime.ScriptMode();
            const auto interpreter = runtime.CreateInterpreter(request);
            if (interpreter != 0u && runtime.AddScriptModule(
                    runtime.ExecutorAddress() + 84u, interpreter)) {
                const auto context = runtime.ExecutorAddress() + 84u;
                const auto function_name = runtime.FunctionName(interpreter);
                const auto module_path = runtime.FullPath(interpreter);
                runtime.PushStackModule(context, module_path, function_name);
                runtime.ReleaseInterpreter(interpreter);
            } else {
                runtime.ShowMissingScriptMessage();
            }
        }

        runtime.CleanupAllScripts();
        full_ready = false;
        (void)runtime.CleanupString(full_path);
        path_ready = false;
        return runtime.CleanupString(script_path);
    } catch (...) {
        if (full_ready) (void)runtime.CleanupString(full_path);
        if (path_ready) (void)runtime.CleanupString(script_path);
        throw;
    }
}

}  // namespace pal4
