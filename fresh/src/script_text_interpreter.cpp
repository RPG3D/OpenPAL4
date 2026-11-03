#include "pal4/script_text_interpreter.h"

namespace pal4 {

ScriptTextInterpreterState* ScriptTextInterpreter::Initialize(
    ScriptTextInterpreterState& state, const void* base_vtable,
    const void* text_vtable) {
    ScriptInterpreterBase::Initialize(state.base, base_vtable);
    state.base.vtable = text_vtable;
    state.source = nullptr;
    state.source_size = 0u;
    return &state;
}

void ScriptTextInterpreter::ReleaseFields(
    ScriptTextInterpreterState& state, const void* base_vtable,
    const void* text_vtable, IScriptTextInterpreterRuntime& runtime) {
    state.base.vtable = text_vtable;
    if (state.source) {
        runtime.Free(state.source);
        state.source = nullptr;
    }
    state.source_size = 0u;
    ScriptInterpreterBase::ReleaseFields(state.base, base_vtable, runtime);
}

bool ScriptTextInterpreter::LoadSource(
    ScriptTextInterpreterState& state, const char* source,
    std::uint32_t count, IScriptTextInterpreterRuntime& runtime) {
    if (!source) {
        return false;
    }
    if (state.source) {
        runtime.Free(state.source);
        state.source = nullptr;
    }
    state.source_size = count;
    state.source = static_cast<char*>(runtime.Allocate(count));
    runtime.CopyCount(state.source, source, count);
    return true;
}

bool ScriptTextInterpreter::LoadFile(
    ScriptTextInterpreterState& state, const char* path, std::uint32_t,
    IScriptTextInterpreterRuntime& runtime) {
    if (!path) {
        return false;
    }
    void* stream = runtime.OpenBinary(path);
    if (!stream) {
        return false;
    }
    if (state.source) {
        runtime.Free(state.source);
        state.source = nullptr;
    }
    state.source_size = static_cast<std::uint32_t>(runtime.FileLength(stream));
    state.source = static_cast<char*>(runtime.Allocate(state.source_size));
    (void)runtime.Read(state.source, state.source_size, 1u, stream);
    runtime.Close(stream);
    return true;
}

const char* ScriptTextInterpreter::GetSourceOrEmpty(
    const ScriptTextInterpreterState& state, const char* empty) {
    return state.source ? state.source : empty;
}

std::uint32_t ScriptTextInterpreter::GetSourceSizeOrZero(
    const ScriptTextInterpreterState& state) {
    return state.source ? state.source_size : 0u;
}

}  // namespace pal4
