#include "pal4/script_execution_context.h"

#include "pal4/script_wait_handler_base.h"

#include <cstring>

namespace pal4 {

int ScriptExecutionContext::ProcessWaitHandlers(
    ScriptExecutionContextState& state, int first, int second,
    IScriptHandlerDispatchRuntime& runtime) {
    auto* cursor = static_cast<ScriptWaitHandlerBaseState**>(state.buffer_36);
    auto* end = static_cast<ScriptWaitHandlerBaseState**>(state.buffer_40);
    while (cursor != end) {
        auto* handler = *cursor;
        runtime.Process(*handler, first, second);
        if (ScriptWaitHandlerBase::IsClosed(*handler) == 0) {
            ++cursor;
            continue;
        }
        runtime.Delete(*handler, 1);
        std::memmove(cursor, cursor + 1,
                     static_cast<std::size_t>(end - (cursor + 1)) *
                         sizeof(*cursor));
        --end;
        state.buffer_40 = end;
        if (cursor == end) {
            ClearExecutionFlag(state);
            return runtime.ExecuteCurrentModule(state);
        }
    }
    return runtime.ExecuteCurrentModule(state);
}

}  // namespace pal4
