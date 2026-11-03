#include "pal4/script_execution_context.h"

#include <cstring>
#if !defined(_WIN32)
#include <strings.h>
#endif

namespace pal4 {

int ScriptExecutionContext::CompleteStackCleanup(
    ScriptExecutionContextState& state,
    IScriptExecutionCleanupRuntime& runtime) {
    ClearStack(state, runtime);
    return CleanupHandlers(state, runtime);
}

int ScriptExecutionContext::ClearStack(
    ScriptExecutionContextState& state,
    IScriptExecutionCleanupRuntime& runtime) {
    auto* begin = static_cast<std::byte*>(state.buffer_20);
    auto* end = static_cast<std::byte*>(state.buffer_24);
    constexpr auto width = sizeof(ScriptStackModuleRecord);
    while (begin != nullptr && end != begin) {
        auto* record = reinterpret_cast<ScriptStackModuleRecord*>(end - width);
        runtime.StopScriptContext(record->script_context);
        runtime.ReleaseScriptContext(record->script_context);
        record->script_context = 0;
        end -= width;
        state.buffer_24 = end;
    }
    state.current_stack_record = 0;
    return 0;
}

int ScriptExecutionContext::RemoveOtherStackModules(
    ScriptExecutionContextState& state,
    IScriptExecutionCleanupRuntime& runtime) {
    auto* cursor = static_cast<std::byte*>(state.buffer_20);
    auto* end = static_cast<std::byte*>(state.buffer_24);
    constexpr auto width = sizeof(ScriptStackModuleRecord);
    while (cursor != end) {
        const auto* current_name = reinterpret_cast<const char*>(
            static_cast<std::uintptr_t>(state.current_stack_record));
#if defined(_WIN32)
        const int comparison = _stricmp(current_name,
                                        reinterpret_cast<const char*>(cursor));
#else
        const int comparison = strcasecmp(
            current_name, reinterpret_cast<const char*>(cursor));
#endif
        if (comparison == 0) {
            cursor += width;
            continue;
        }

        auto* record = reinterpret_cast<ScriptStackModuleRecord*>(cursor);
        runtime.StopScriptContext(record->script_context);
        runtime.ReleaseScriptContext(record->script_context);
        record->script_context = 0;
        std::memmove(cursor, cursor + width,
                     static_cast<std::size_t>(end - (cursor + width)));
        end -= width;
        state.buffer_24 = end;
    }
    return CleanupHandlers(state, runtime);
}

int ScriptExecutionContext::CleanupHandlers(
    ScriptExecutionContextState& state,
    IScriptExecutionCleanupRuntime& runtime) {
    auto* cursor = static_cast<void**>(state.buffer_36);
    auto* end = static_cast<void**>(state.buffer_40);
    while (cursor != end) {
        void* handler = *cursor++;
        runtime.ResetHandler(handler);
        runtime.DeleteHandler(handler, 1);
    }
    state.buffer_40 = state.buffer_36;
    return static_cast<int>(reinterpret_cast<std::uintptr_t>(
        ClearExecutionFlag(state)));
}

}  // namespace pal4
