#include "pal4/script_execution_context.h"

namespace pal4 {

int ScriptExecutionContext::InitializeEngine(
    ScriptExecutionContextState& state,
    ScriptEngineRegistrationCallback callback, std::uint32_t context_word,
    IScriptEngineBoundary& runtime) {
    void* engine = runtime.CreateEngine(20200);
    state.script_engine = engine;
    state.word_8 = context_word;
    runtime.RegisterStringFunctions(engine);
    runtime.RegisterMathFunctions(engine);
    runtime.RegisterGameFunctions(engine);
    return callback ? runtime.InvokeRegistration(callback, engine) : 0;
}

int ScriptExecutionContext::CleanupEngine(
    ScriptExecutionContextState& state, IScriptEngineBoundary& runtime) {
    state.word_8 = 0;
    int result = runtime.CompleteCacheCleanup(state);
    if (state.script_engine) {
        result = runtime.ReleaseEngine(state.script_engine);
        state.script_engine = nullptr;
    }
    return result;
}

void* ScriptExecutionContext::GetEngine(
    const ScriptExecutionContextState& state) {
    return state.script_engine;
}

}  // namespace pal4
