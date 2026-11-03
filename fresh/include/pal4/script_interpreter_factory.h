#pragma once

#include "pal4/script_interpreter_base.h"

#include <cstdint>

namespace pal4 {

class IScriptInterpreterFactoryRuntime : public IScriptInterpreterBaseRuntime {
public:
    virtual ScriptInterpreterBaseState* ConstructText(void* memory) = 0;
    virtual ScriptInterpreterBaseState* ConstructBinary(void* memory) = 0;
    virtual bool Initialize(ScriptInterpreterBaseState* interpreter,
                            std::uint32_t context,
                            std::uint32_t callback) = 0;
    virtual void Destroy(ScriptInterpreterBaseState* interpreter,
                         unsigned flags) = 0;
};

class ScriptInterpreterFactory final {
public:
    static ScriptInterpreterBaseState* Create(
        int script_type, const char* path, const char* full_path,
        const char* name, std::uint32_t context, std::uint32_t callback,
        IScriptInterpreterFactoryRuntime& runtime);
    static ScriptInterpreterBaseState* Release(
        ScriptInterpreterBaseState* interpreter,
        IScriptInterpreterFactoryRuntime& runtime);
};

}  // namespace pal4
