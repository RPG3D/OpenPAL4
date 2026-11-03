#pragma once

#include "pal4/script_interpreter_base.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ScriptTextInterpreterState {
    ScriptInterpreterBaseState base{};
    char* source{};
    std::uint32_t source_size{};
};

#if UINTPTR_MAX == 0xffffffffu
static_assert(sizeof(ScriptTextInterpreterState) == 28);
static_assert(offsetof(ScriptTextInterpreterState, source) == 20);
static_assert(offsetof(ScriptTextInterpreterState, source_size) == 24);
#endif

class IScriptTextInterpreterRuntime : public IScriptInterpreterBaseRuntime {
public:
    virtual void CopyCount(char* destination, const char* source,
                           std::size_t count) = 0;
    virtual void* OpenBinary(const char* path) = 0;
    virtual std::int32_t FileLength(void* stream) = 0;
    virtual std::size_t Read(void* destination, std::size_t element_size,
                             std::size_t element_count, void* stream) = 0;
    virtual void Close(void* stream) = 0;
};

class ScriptTextInterpreter final {
public:
    static ScriptTextInterpreterState* Initialize(
        ScriptTextInterpreterState& state, const void* base_vtable,
        const void* text_vtable);
    static void ReleaseFields(ScriptTextInterpreterState& state,
                              const void* base_vtable,
                              const void* text_vtable,
                              IScriptTextInterpreterRuntime& runtime);
    static bool LoadSource(ScriptTextInterpreterState& state, const char* source,
                           std::uint32_t count,
                           IScriptTextInterpreterRuntime& runtime);
    static bool LoadFile(ScriptTextInterpreterState& state, const char* path,
                         std::uint32_t unused,
                         IScriptTextInterpreterRuntime& runtime);
    [[nodiscard]] static const char* GetSourceOrEmpty(
        const ScriptTextInterpreterState& state, const char* empty);
    [[nodiscard]] static std::uint32_t GetSourceSizeOrZero(
        const ScriptTextInterpreterState& state);
};

}  // namespace pal4
