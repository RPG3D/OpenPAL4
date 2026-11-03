#include "pal4/script_execution_context.h"

#include <string>

namespace pal4 {

namespace {

ScriptStackModuleRecord* Current(ScriptExecutionContextState& state) {
    return reinterpret_cast<ScriptStackModuleRecord*>(
        static_cast<std::uintptr_t>(state.current_stack_record));
}

std::string ExceptionText(ScriptExecutionContextState& state,
                          ScriptStackModuleRecord& record,
                          IScriptCurrentModuleRuntime& runtime) {
    const char* function_name = runtime.ExceptionFunctionName(
        record.script_context);
    const int line = runtime.ExceptionLine(record.script_context, 0);
    const char* reason = runtime.ExceptionReason(record.script_context, 0);
    const char* declaration = runtime.FunctionDeclaration(
        state.script_engine, function_name, 0);
    std::string text = "< catched an exception >\r\nreason:  ";
    text += reason;
    text += "\r\nfunc:    ";
    text += declaration;
    text += function_name;
    text += "\r\nline:    ";
    text += std::to_string(line);
    text += "\r\n";
    return text;
}

}  // namespace

int ScriptExecutionContext::ExecuteCurrentModule(
    ScriptExecutionContextState& state,
    IScriptCurrentModuleRuntime& runtime) {
    auto* record = Current(state);
    if (record == nullptr) return -1;

    if (record->function_index == 0) {
        auto* begin = static_cast<std::byte*>(state.buffer_20);
        auto* end = static_cast<std::byte*>(state.buffer_24);
        constexpr auto width = sizeof(ScriptStackModuleRecord);
        if (begin == nullptr || begin == end) {
            runtime.CompleteCleanup(state);
            return 0;
        }
        if (end - width == reinterpret_cast<std::byte*>(record)) {
            runtime.StopContext(record->script_context);
            runtime.ReleaseContext(record->script_context);
            record->script_context = 0;
            runtime.RemoveModule(state.script_engine, *record);
            end -= width;
            state.buffer_24 = end;
            if (begin == end) {
                runtime.CompleteCleanup(state);
                return 0;
            }
            state.current_stack_record = static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(end - width));
        }
    }

    record = Current(state);
    if (record->execution_flag != 0) {
        runtime.AbortContext(record->script_context);
        Current(state)->function_index = 1;
        return 1;
    }

    if (record->word_268 == 1) {
        while (Current(state)->function_index != 0 &&
               Current(state)->word_268 != 0) {
            record = Current(state);
            record->function_index = runtime.ExecuteContext(
                record->script_context, 0);
        }
    } else {
        record->function_index = runtime.ExecuteContext(
            record->script_context, 0);
    }

    record = Current(state);
    if (record->function_index == -1) return -1;
    if (record->function_index == 3) {
        std::string text = ExceptionText(state, *record, runtime);
        if (state.word_8 != 0) {
            runtime.ReportException(state.word_8, text);
        } else {
            runtime.RaiseException(text);
        }
    }
    return Current(state)->function_index;
}

}  // namespace pal4
