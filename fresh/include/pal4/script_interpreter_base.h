#pragma once

#include <cstddef>

namespace pal4 {

struct ScriptInterpreterBaseState {
    const void* vtable = nullptr;
    char* full_path = nullptr;
    char* path = nullptr;
    char* name = nullptr;
    char* extra = nullptr;
};

class IScriptInterpreterBaseRuntime {
public:
    virtual ~IScriptInterpreterBaseRuntime() = default;
    virtual void* Allocate(std::size_t bytes) = 0;
    virtual void Free(void* pointer) = 0;
    virtual std::size_t Length(const char* text) = 0;
    virtual void Copy(char* destination, const char* source) = 0;
};

class ScriptInterpreterBase final {
public:
    static ScriptInterpreterBaseState* Initialize(
        ScriptInterpreterBaseState& state, const void* base_vtable);
    static void ReleaseFields(ScriptInterpreterBaseState& state,
                              const void* base_vtable,
                              IScriptInterpreterBaseRuntime& runtime);
    static ScriptInterpreterBaseState* DestroyAt(
        ScriptInterpreterBaseState* state, unsigned flags,
        const void* base_vtable, IScriptInterpreterBaseRuntime& runtime);
    static bool SetFullPath(ScriptInterpreterBaseState& state,
                            const char* text,
                            IScriptInterpreterBaseRuntime& runtime);
    static bool SetPath(ScriptInterpreterBaseState& state, const char* text,
                        IScriptInterpreterBaseRuntime& runtime);
    static bool SetName(ScriptInterpreterBaseState& state, const char* text,
                        IScriptInterpreterBaseRuntime& runtime);
    static const char* GetFullPathOrEmpty(
        const ScriptInterpreterBaseState& state, const char* empty);
    static const char* GetPathOrEmpty(
        const ScriptInterpreterBaseState& state, const char* empty);
    static const char* GetNameOrEmpty(
        const ScriptInterpreterBaseState& state, const char* empty);
    static bool ReturnFalseTwoArgs(unsigned first, unsigned second);
    static const char* GetEmptyString(const char* empty);
    static int ReturnMinusOne();
    static int ReturnTwoShared();
};

}  // namespace pal4
