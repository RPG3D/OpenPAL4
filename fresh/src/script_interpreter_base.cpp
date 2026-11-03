#include "pal4/script_interpreter_base.h"

namespace pal4 {
namespace {

bool ReplaceText(char*& slot, const char* text,
                 IScriptInterpreterBaseRuntime& runtime) {
    if (!text) {
        return false;
    }
    if (slot) {
        runtime.Free(slot);
        slot = nullptr;
    }
    char* replacement = static_cast<char*>(runtime.Allocate(runtime.Length(text) + 1));
    slot = replacement;
    runtime.Copy(replacement, text);
    return true;
}

void ReleaseText(char*& slot, IScriptInterpreterBaseRuntime& runtime) {
    if (slot) {
        runtime.Free(slot);
        slot = nullptr;
    }
}

}  // namespace

ScriptInterpreterBaseState* ScriptInterpreterBase::Initialize(
    ScriptInterpreterBaseState& state, const void* base_vtable) {
    state.full_path = nullptr;
    state.path = nullptr;
    state.name = nullptr;
    state.extra = nullptr;
    state.vtable = base_vtable;
    return &state;
}

void ScriptInterpreterBase::ReleaseFields(
    ScriptInterpreterBaseState& state, const void* base_vtable,
    IScriptInterpreterBaseRuntime& runtime) {
    state.vtable = base_vtable;
    ReleaseText(state.full_path, runtime);
    ReleaseText(state.path, runtime);
    ReleaseText(state.name, runtime);
    ReleaseText(state.extra, runtime);
}

ScriptInterpreterBaseState* ScriptInterpreterBase::DestroyAt(
    ScriptInterpreterBaseState* state, unsigned flags,
    const void* base_vtable, IScriptInterpreterBaseRuntime& runtime) {
    ReleaseFields(*state, base_vtable, runtime);
    if ((flags & 1u) != 0) {
        runtime.Free(state);
    }
    return state;
}

bool ScriptInterpreterBase::SetFullPath(
    ScriptInterpreterBaseState& state, const char* text,
    IScriptInterpreterBaseRuntime& runtime) {
    return ReplaceText(state.full_path, text, runtime);
}

bool ScriptInterpreterBase::SetPath(
    ScriptInterpreterBaseState& state, const char* text,
    IScriptInterpreterBaseRuntime& runtime) {
    return ReplaceText(state.path, text, runtime);
}

bool ScriptInterpreterBase::SetName(
    ScriptInterpreterBaseState& state, const char* text,
    IScriptInterpreterBaseRuntime& runtime) {
    return ReplaceText(state.name, text, runtime);
}

const char* ScriptInterpreterBase::GetFullPathOrEmpty(
    const ScriptInterpreterBaseState& state, const char* empty) {
    return state.full_path ? state.full_path : empty;
}

const char* ScriptInterpreterBase::GetPathOrEmpty(
    const ScriptInterpreterBaseState& state, const char* empty) {
    return state.path ? state.path : empty;
}

const char* ScriptInterpreterBase::GetNameOrEmpty(
    const ScriptInterpreterBaseState& state, const char* empty) {
    return state.name ? state.name : empty;
}

bool ScriptInterpreterBase::ReturnFalseTwoArgs(unsigned, unsigned) {
    return false;
}

const char* ScriptInterpreterBase::GetEmptyString(const char* empty) {
    return empty;
}

int ScriptInterpreterBase::ReturnMinusOne() {
    return -1;
}

int ScriptInterpreterBase::ReturnTwoShared() {
    return 2;
}

}  // namespace pal4
