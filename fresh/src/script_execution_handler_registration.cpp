#include "pal4/script_execution_context.h"

#include "pal4/script_wait_handler_base.h"

#include <cstring>

namespace pal4 {

std::uint32_t ScriptExecutionContext::AddWaitHandler(
    ScriptExecutionContextState& state,
    ScriptWaitHandlerBaseState& handler,
    IScriptHandlerRegistrationRuntime& runtime) {
    auto* begin = static_cast<ScriptWaitHandlerBaseState**>(state.buffer_36);
    auto* end = static_cast<ScriptWaitHandlerBaseState**>(state.buffer_40);
    auto* capacity = static_cast<ScriptWaitHandlerBaseState**>(state.buffer_44);

    if (handler.marker >= 0) {
        for (auto* cursor = begin; cursor != end; ++cursor) {
            auto* existing = *cursor;
            if (existing->marker < 0 || handler.marker != existing->marker ||
                !runtime.SameHandlerKey(handler, *existing)) {
                continue;
            }
            runtime.ResetHandler(*existing);
            runtime.DeleteHandler(*existing, 1);
            std::memmove(cursor, cursor + 1,
                         static_cast<std::size_t>(end - (cursor + 1)) *
                             sizeof(*cursor));
            --end;
            state.buffer_40 = end;
            break;
        }
    }

    if (begin != nullptr && end != capacity) {
        *end++ = &handler;
        state.buffer_40 = end;
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(end));
    }

    const std::size_t count = begin ?
        static_cast<std::size_t>(end - begin) : 0;
    const std::size_t growth = count > 1 ? count : 1;
    const std::size_t new_capacity = count + growth;
    auto* replacement = static_cast<ScriptWaitHandlerBaseState**>(
        runtime.AllocatePointers(new_capacity * sizeof(ScriptWaitHandlerBaseState*)));
    if (count != 0) {
        std::memcpy(replacement, begin, count * sizeof(*replacement));
    }
    replacement[count] = &handler;
    runtime.FreePointers(begin);
    state.buffer_36 = replacement;
    state.buffer_40 = replacement + count + 1;
    state.buffer_44 = replacement + new_capacity;
    return static_cast<std::uint32_t>(count);
}

}  // namespace pal4
