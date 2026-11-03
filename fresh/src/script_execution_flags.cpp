#include "pal4/script_execution_context.h"

namespace pal4 {

namespace {

ScriptStackModuleRecord* Current(ScriptExecutionContextState& state) {
    return reinterpret_cast<ScriptStackModuleRecord*>(
        static_cast<std::uintptr_t>(state.current_stack_record));
}

}  // namespace

ScriptStackModuleRecord* ScriptExecutionContext::SetExecutionFlag(
    ScriptExecutionContextState& state) {
    auto* record = Current(state);
    if (record != nullptr && record->function_index != 0) {
        record->execution_flag = 1;
    }
    return record;
}

ScriptStackModuleRecord* ScriptExecutionContext::ClearExecutionFlag(
    ScriptExecutionContextState& state) {
    auto* record = Current(state);
    if (record != nullptr && record->function_index != 0) {
        record->execution_flag = 0;
    }
    return record;
}

}  // namespace pal4
