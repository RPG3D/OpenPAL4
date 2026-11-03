#include "pal4/script_wait_handler_base.h"

namespace pal4 {

ScriptWaitHandlerBaseState* ScriptWaitHandlerBase::Initialize(
    ScriptWaitHandlerBaseState& state, std::uint8_t wait_mode,
    void* script_context, const void* base_vtable) {
    state.wait_mode = wait_mode;
    state.script_context = script_context;
    state.closed = 0;
    state.marker = -1;
    state.mode_copy = wait_mode;
    state.text_data = nullptr;
    state.text_end = nullptr;
    state.text_capacity = nullptr;
    state.vtable = base_vtable;
    return &state;
}

ScriptWaitHandlerBaseState* ScriptWaitHandlerBase::DestroyAt(
    ScriptWaitHandlerBaseState* state, unsigned flags,
    const void* base_vtable, IScriptWaitHandlerBaseRuntime& runtime) {
    ReleaseFields(*state, base_vtable, runtime);
    if ((flags & 1u) != 0) {
        runtime.Free(state);
    }
    return state;
}

void ScriptWaitHandlerBase::ReleaseFields(
    ScriptWaitHandlerBaseState& state, const void* base_vtable,
    IScriptWaitHandlerBaseRuntime& runtime) {
    state.vtable = base_vtable;
    state.script_context = nullptr;
    if (state.text_data) {
        std::uint8_t* header = state.text_data - 1;
        const std::uint8_t references = *header;
        if (references != 0 && references != 0xff) {
            *header = static_cast<std::uint8_t>(references - 1);
        } else {
            runtime.Free(header);
        }
    }
    state.text_data = nullptr;
    state.text_end = nullptr;
    state.text_capacity = nullptr;
}

std::uint8_t ScriptWaitHandlerBase::Flush(
    ScriptWaitHandlerBaseState& state, IScriptWaitHandlerBaseRuntime& runtime) {
    state.closed = 0;
    if (state.wait_mode != 0 && state.script_context) {
        return runtime.SetExecutionFlag(state.script_context);
    }
    return state.wait_mode;
}

void ScriptWaitHandlerBase::Close(ScriptWaitHandlerBaseState& state) {
    state.closed = 1;
}

std::uint8_t ScriptWaitHandlerBase::IsClosed(
    const ScriptWaitHandlerBaseState& state) {
    return state.closed;
}

}  // namespace pal4
