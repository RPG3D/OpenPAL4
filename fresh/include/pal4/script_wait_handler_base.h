#pragma once

#include <cstdint>

namespace pal4 {

struct ScriptWaitHandlerBaseState {
    const void* vtable;
    std::uint8_t closed;
    std::uint8_t wait_mode;
    std::uint8_t reserved_6[2];
    void* script_context;
    std::int32_t marker;
    std::uint8_t mode_copy;
    std::uint8_t reserved_17[3];
    std::uint8_t* text_data;
    std::uint8_t* text_end;
    std::uint8_t* text_capacity;
};

class IScriptWaitHandlerBaseRuntime {
public:
    virtual ~IScriptWaitHandlerBaseRuntime() = default;
    virtual void Free(void* pointer) = 0;
    virtual std::uint8_t SetExecutionFlag(void* script_context) = 0;
};

class ScriptWaitHandlerBase final {
public:
    static ScriptWaitHandlerBaseState* Initialize(
        ScriptWaitHandlerBaseState& state, std::uint8_t wait_mode,
        void* script_context, const void* base_vtable);
    static ScriptWaitHandlerBaseState* DestroyAt(
        ScriptWaitHandlerBaseState* state, unsigned flags,
        const void* base_vtable, IScriptWaitHandlerBaseRuntime& runtime);
    static void ReleaseFields(ScriptWaitHandlerBaseState& state,
                              const void* base_vtable,
                              IScriptWaitHandlerBaseRuntime& runtime);
    static std::uint8_t Flush(ScriptWaitHandlerBaseState& state,
                              IScriptWaitHandlerBaseRuntime& runtime);
    static void Close(ScriptWaitHandlerBaseState& state);
    static std::uint8_t IsClosed(const ScriptWaitHandlerBaseState& state);
};

}  // namespace pal4
