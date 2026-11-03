#include "pal4/script_binary_interpreter.h"

#include <cstring>

namespace pal4 {

ScriptBinaryDataState* ScriptBinaryDataStream::Initialize(
    ScriptBinaryDataState& state, const void* vtable) {
    state.read_offset = 0u;
    state.write_offset = 0u;
    state.buffer_ptr = nullptr;
    state.total_size = 0u;
    state.vtable = vtable;
    return &state;
}

void ScriptBinaryDataStream::ReleaseFields(
    ScriptBinaryDataState& state, const void* vtable,
    IScriptInterpreterBaseRuntime& runtime) {
    state.vtable = vtable;
    if (state.buffer_ptr) {
        runtime.Free(state.buffer_ptr);
        state.buffer_ptr = nullptr;
    }
}

std::uint32_t ScriptBinaryDataStream::Append(
    ScriptBinaryDataState& state, const void* source, std::uint32_t size,
    IScriptInterpreterBaseRuntime& runtime) {
    void* temporary = nullptr;
    if (!state.buffer_ptr) {
        state.buffer_ptr = runtime.Allocate(size);
    } else {
        // The original copies through a temporary allocation before growing.
        void* old_buffer = state.buffer_ptr;
        temporary = runtime.Allocate(state.total_size);
        if (state.total_size) {
            std::memcpy(temporary, old_buffer, state.total_size);
        }
        runtime.Free(old_buffer);
        state.buffer_ptr = runtime.Allocate(state.total_size + size);
        if (state.total_size) {
            std::memcpy(state.buffer_ptr, temporary, state.total_size);
        }
    }
    if (size) {
        std::memcpy(static_cast<char*>(state.buffer_ptr) + state.write_offset,
                    source, size);
    }
    if (temporary) {
        runtime.Free(temporary);
    }
    state.total_size += size;
    state.write_offset += size;
    return state.write_offset;
}

void ScriptBinaryDataStream::Read(ScriptBinaryDataState& state,
                                  void* destination, std::uint32_t size) {
    if (size) {
        std::memcpy(destination,
                    static_cast<const char*>(state.buffer_ptr) + state.read_offset,
                    size);
    }
    state.read_offset += size;
}

ScriptBinaryInterpreterState* ScriptBinaryInterpreter::Initialize(
    ScriptBinaryInterpreterState& state, const void* base_vtable,
    const void* data_vtable, const void* binary_vtable) {
    ScriptInterpreterBase::Initialize(state.base, base_vtable);
    ScriptBinaryDataStream::Initialize(state.data, data_vtable);
    state.base.vtable = binary_vtable;
    return &state;
}

void ScriptBinaryInterpreter::ReleaseFields(
    ScriptBinaryInterpreterState& state, const void* base_vtable,
    const void* data_vtable, const void* binary_vtable,
    IScriptInterpreterBaseRuntime& runtime) {
    state.base.vtable = binary_vtable;
    ScriptBinaryDataStream::ReleaseFields(state.data, data_vtable, runtime);
    ScriptInterpreterBase::ReleaseFields(state.base, base_vtable, runtime);
}

bool ScriptBinaryInterpreter::AppendBuffer(
    ScriptBinaryInterpreterState& state, const void* source, std::uint32_t size,
    IScriptBinaryDataRuntime& runtime) {
    if (!source) {
        return false;
    }
    runtime.AppendData(state.data, source, size);
    return true;
}

std::uint8_t ScriptBinaryInterpreter::ResetAndLoad(
    ScriptBinaryInterpreterState& state, const void* source,
    ScriptBinaryLoadCallback callback, IScriptInterpreterBaseRuntime& runtime) {
    if (!source) {
        return 0u;
    }
    if (state.data.buffer_ptr) {
        runtime.Free(state.data.buffer_ptr);
        state.data.buffer_ptr = nullptr;
    }
    state.data.read_offset = 0u;
    state.data.write_offset = 0u;
    state.data.total_size = 0u;
    return static_cast<std::uint8_t>(
        callback(source, &state.data.total_size, &state.data.buffer_ptr));
}

void* ScriptBinaryInterpreter::GetBuffer(
    const ScriptBinaryInterpreterState& state) {
    return state.data.buffer_ptr;
}

std::uint32_t ScriptBinaryInterpreter::GetSize(
    const ScriptBinaryInterpreterState& state) {
    return state.data.total_size;
}

}  // namespace pal4
