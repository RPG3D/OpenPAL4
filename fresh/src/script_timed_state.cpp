#include "pal4/script_timed_state.h"

namespace pal4 {

std::uint32_t ScriptTimedState::Start(
    const std::uint32_t value, const std::uint32_t callback_record,
    IScriptTimedStateRuntime& runtime) {
    auto callback = runtime.ReadWord(callback_record + 4u);
    if (callback == 0u) callback = runtime.EmptyString();
    return runtime.InitializeState(runtime.ScriptExecutor(), value, callback);
}

std::uint32_t ScriptTimedState::Terminate(
    IScriptTimedStateRuntime& runtime) {
    const auto executor = runtime.ScriptExecutor();
    const auto empty = runtime.EmptyString();
    runtime.WriteWord(executor + 0x3cu, 0u);
    const auto result = runtime.InitializeText(
        executor + 0x48u, empty, runtime.TextLength(empty));
    runtime.WriteWord(executor + 0x40u, 0u);
    runtime.WriteWord(executor + 0x44u, 0u);
    return result;
}

}  // namespace pal4
