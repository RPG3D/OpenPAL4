#include "pal4/script_execution_context.h"

#include <cstring>
#if !defined(_WIN32)
#include <strings.h>
#endif

namespace pal4 {

namespace {

constexpr char kMissingModule[] =
    "error<executor:pushStackModule>: null ModuleName\r\n";
constexpr char kMissingFunction[] =
    "error<executor:pushStackModule>: null scriptFunc\r\n";
constexpr char kMissingEngine[] =
    "error<executor::pushStackModule>: null scriptEngine\r\n";
constexpr char kMissingContextReport[] =
    "error<executor::pushStackModule>: null insertContext\r\n";
constexpr char kMissingContextThrow[] =
    "error<executor::pushStackModule>: null scriptContext\r\n";
constexpr char kNoFunction[] =
    "error<executor:pushStackModule>: no Function\r\n";
constexpr char kPrepareFailure[] =
    "error<executor:pushStackModule>: Prepare Function Error\r\n";

int Fail(std::uint32_t logger, const char* message,
         IScriptStackModuleRuntime& runtime, int result = -1,
         const char* throw_message = nullptr) {
    if (logger != 0) {
        runtime.Report(logger, message);
    } else {
        runtime.RaiseError(throw_message ? throw_message : message);
    }
    return result;
}

void AppendRecord(ScriptExecutionContextState& state,
                  const ScriptStackModuleRecord& record,
                  IScriptStackModuleRuntime& runtime) {
    auto* begin = static_cast<std::byte*>(state.buffer_20);
    auto* end = static_cast<std::byte*>(state.buffer_24);
    auto* capacity = static_cast<std::byte*>(state.buffer_28);
    constexpr auto width = sizeof(ScriptStackModuleRecord);

    if (begin == nullptr || end == capacity) {
        const std::size_t count = begin ?
            static_cast<std::size_t>(end - begin) / width : 0;
        const std::size_t growth = count > 1 ? count : 1;
        const std::size_t new_capacity = count + growth;
        auto* replacement = static_cast<std::byte*>(
            runtime.AllocateRecords(new_capacity * width));
        if (count != 0) {
            std::memcpy(replacement, begin, count * width);
        }
        runtime.FreeRecords(begin);
        begin = replacement;
        end = begin + count * width;
        capacity = begin + new_capacity * width;
        state.buffer_20 = begin;
        state.buffer_28 = capacity;
    }

    std::memcpy(end, &record, width);
    end += width;
    state.buffer_24 = end;
    state.current_stack_record = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(end - width));
}

}  // namespace

int ScriptExecutionContext::PushStackModule(
    ScriptExecutionContextState& state, const char* module,
    const char* function, IScriptStackModuleRuntime& runtime) {
    if (module == nullptr || module[0] == '\0') {
        return Fail(state.word_8, kMissingModule, runtime);
    }
    if (function == nullptr || function[0] == '\0') {
        return Fail(state.word_8, kMissingFunction, runtime);
    }
    if (state.script_engine == nullptr) {
        return Fail(state.word_8, kMissingEngine, runtime);
    }

    ScriptStackModuleRecord record{};
    runtime.CopyModuleName(record.module_name, module);
    record.function_index = -1;
    runtime.AcquireContext(state.script_engine, record.script_context);
    if (record.script_context == 0) {
        return Fail(state.word_8, kMissingContextReport, runtime,
                    -1, kMissingContextThrow);
    }

    const int index = runtime.FindFunction(
        state.script_engine, module, function);
    if (index == -6) {
        runtime.ReleaseContext(record.script_context);
        if (state.word_8 != 0) runtime.Report(state.word_8, kNoFunction);
        return -6;
    }

    const int translated = runtime.TranslateFunction(state.script_engine,
                                                     index);
    const auto resolved = runtime.ResolveFunction(
        state.script_engine, module, translated);
    if (runtime.Prepare(record.script_context, resolved) != 0) {
        runtime.ReleaseContext(record.script_context);
        if (state.word_8 != 0) runtime.Report(state.word_8, kPrepareFailure);
        return -1;
    }

    if (state.current_stack_record != 0) {
        const auto* previous = reinterpret_cast<const ScriptStackModuleRecord*>(
            static_cast<std::uintptr_t>(state.current_stack_record));
        runtime.StopPrevious(previous->script_context);
        runtime.CleanupHandlers(state);
    }
    AppendRecord(state, record, runtime);
    return 0;
}

bool ScriptExecutionContext::HasStackModule(
    const ScriptExecutionContextState& state, const char* module) {
    auto* cursor = static_cast<const std::byte*>(state.buffer_20);
    const auto* end = static_cast<const std::byte*>(state.buffer_24);
    while (cursor != end) {
        const auto* record = reinterpret_cast<const ScriptStackModuleRecord*>(
            cursor);
#if defined(_WIN32)
        const int comparison = _stricmp(record->module_name, module);
#else
        const int comparison = strcasecmp(record->module_name, module);
#endif
        if (comparison == 0) return true;
        cursor += sizeof(ScriptStackModuleRecord);
    }
    return false;
}

int ScriptExecutionContext::FindStackModuleStatus(
    const ScriptExecutionContextState& state, const char* module) {
    auto* cursor = static_cast<const std::byte*>(state.buffer_20);
    const auto* end = static_cast<const std::byte*>(state.buffer_24);
    while (cursor != end) {
        const auto* record = reinterpret_cast<const ScriptStackModuleRecord*>(
            cursor);
        const char* left = record->module_name;
        const char* right = module;
        while (*left != '\0' && *left == *right) {
            ++left;
            ++right;
        }
        if (*left == *right) return record->function_index;
        cursor += sizeof(ScriptStackModuleRecord);
    }
    return 0;
}

int ScriptExecutionContext::StackEntryCount(
    const ScriptExecutionContextState& state) {
    if (state.buffer_20 == nullptr) return 0;
    return static_cast<int>(
        (static_cast<const std::byte*>(state.buffer_24) -
         static_cast<const std::byte*>(state.buffer_20)) /
        sizeof(ScriptStackModuleRecord));
}

ScriptStackModuleRecord* ScriptExecutionContext::SetDebuggerMode(
    ScriptExecutionContextState& state, int mode) {
    auto* record = reinterpret_cast<ScriptStackModuleRecord*>(
        static_cast<std::uintptr_t>(state.current_stack_record));
    if (record != nullptr && record->function_index != 0 &&
        mode >= 0 && mode <= 1) {
        record->word_272 = record->word_268;
        record->word_268 = static_cast<std::uint32_t>(mode);
    }
    return record;
}

}  // namespace pal4
