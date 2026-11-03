#include "pal4/script_execution_context.h"

#include <cstddef>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    static_assert(offsetof(pal4::ScriptStackModuleRecord, function_index) ==
                  260);
    static_assert(offsetof(pal4::ScriptStackModuleRecord, execution_flag) ==
                  264);
    pal4::ScriptExecutionContextState state{};
    Check(pal4::ScriptExecutionContext::SetExecutionFlag(state) == nullptr &&
              pal4::ScriptExecutionContext::ClearExecutionFlag(state) == nullptr,
          "missing current record returns null for both operations");

    pal4::ScriptStackModuleRecord record{};
    state.current_stack_record = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&record));
    record.execution_flag = 7;
    Check(pal4::ScriptExecutionContext::SetExecutionFlag(state) == &record &&
              record.execution_flag == 7 &&
              pal4::ScriptExecutionContext::ClearExecutionFlag(state) ==
                  &record && record.execution_flag == 7,
          "zero function index blocks both writes but preserves return pointer");

    record.function_index = -1;
    Check(pal4::ScriptExecutionContext::SetExecutionFlag(state) == &record &&
              record.execution_flag == 1 &&
              pal4::ScriptExecutionContext::ClearExecutionFlag(state) ==
                  &record && record.execution_flag == 0,
          "nonzero function index permits one and zero flag writes");
}
