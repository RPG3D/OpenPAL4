#include "pal4/script_execution_context.h"

#include <cstdint>

namespace pal4 {

ScriptExecutionContextState* ScriptExecutionContext::Initialize(
    ScriptExecutionContextState& state, const void* base_vtable) {
    const auto address_byte = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(&state) >> 24);
    state.script_engine = nullptr;
    state.word_8 = 0;
    state.current_stack_record = 0;
    state.address_byte_16 = address_byte;
    state.buffer_20 = nullptr;
    state.buffer_24 = nullptr;
    state.buffer_28 = nullptr;
    state.address_byte_32 = address_byte;
    state.buffer_36 = nullptr;
    state.buffer_40 = nullptr;
    state.buffer_44 = nullptr;
    state.vtable = base_vtable;
    return &state;
}

ScriptExecutionContextState* ScriptExecutionContext::DestroyAt(
    ScriptExecutionContextState* state, unsigned flags,
    const void* base_vtable, IScriptExecutionContextRuntime& runtime) {
    ReleaseFields(*state, base_vtable, runtime);
    if ((flags & 1u) != 0) {
        runtime.Free(state);
    }
    return state;
}

void ScriptExecutionContext::ReleaseFields(
    ScriptExecutionContextState& state, const void* base_vtable,
    IScriptExecutionContextRuntime& runtime) {
    state.vtable = base_vtable;
    runtime.Cleanup(state);
    runtime.Free(state.buffer_36);
    state.buffer_36 = nullptr;
    state.buffer_40 = nullptr;
    state.buffer_44 = nullptr;
    runtime.Free(state.buffer_20);
    state.buffer_20 = nullptr;
    state.buffer_24 = nullptr;
    state.buffer_28 = nullptr;
}

}  // namespace pal4
