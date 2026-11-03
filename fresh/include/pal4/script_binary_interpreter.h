#pragma once

#include "pal4/script_interpreter_base.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ScriptBinaryDataState {
    const void* vtable{};
    std::uint32_t read_offset{};
    std::uint32_t write_offset{};
    void* buffer_ptr{};
    std::uint32_t total_size{};
};

struct ScriptBinaryInterpreterState {
    ScriptInterpreterBaseState base{};
    ScriptBinaryDataState data{};
};

#if UINTPTR_MAX == 0xffffffffu
static_assert(sizeof(ScriptBinaryDataState) == 20);
static_assert(sizeof(ScriptBinaryInterpreterState) == 40);
static_assert(offsetof(ScriptBinaryInterpreterState, data) == 20);
static_assert(offsetof(ScriptBinaryDataState, buffer_ptr) == 12);
#endif

class IScriptBinaryDataRuntime : public IScriptInterpreterBaseRuntime {
public:
    virtual void AppendData(ScriptBinaryDataState& data, const void* source,
                            std::uint32_t size) = 0;
};

class ScriptBinaryDataStream final {
public:
    static ScriptBinaryDataState* Initialize(ScriptBinaryDataState& state,
                                              const void* vtable);
    static void ReleaseFields(ScriptBinaryDataState& state, const void* vtable,
                              IScriptInterpreterBaseRuntime& runtime);
    static std::uint32_t Append(ScriptBinaryDataState& state, const void* source,
                                std::uint32_t size,
                                IScriptInterpreterBaseRuntime& runtime);
    static void Read(ScriptBinaryDataState& state, void* destination,
                     std::uint32_t size);
};

using ScriptBinaryLoadCallback = std::uint32_t (*)(
    const void* source, std::uint32_t* total_size, void** buffer_ptr);

class ScriptBinaryInterpreter final {
public:
    static ScriptBinaryInterpreterState* Initialize(
        ScriptBinaryInterpreterState& state, const void* base_vtable,
        const void* data_vtable, const void* binary_vtable);
    static void ReleaseFields(ScriptBinaryInterpreterState& state,
                              const void* base_vtable,
                              const void* data_vtable,
                              const void* binary_vtable,
                              IScriptInterpreterBaseRuntime& runtime);
    static bool AppendBuffer(ScriptBinaryInterpreterState& state,
                             const void* source, std::uint32_t size,
                             IScriptBinaryDataRuntime& runtime);
    static std::uint8_t ResetAndLoad(ScriptBinaryInterpreterState& state,
                                     const void* source,
                                     ScriptBinaryLoadCallback callback,
                                     IScriptInterpreterBaseRuntime& runtime);
    static void* GetBuffer(const ScriptBinaryInterpreterState& state);
    static std::uint32_t GetSize(const ScriptBinaryInterpreterState& state);
};

}  // namespace pal4
