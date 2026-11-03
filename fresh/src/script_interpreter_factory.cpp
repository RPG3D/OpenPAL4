#include "pal4/script_interpreter_factory.h"

namespace pal4 {

ScriptInterpreterBaseState* ScriptInterpreterFactory::Create(
    int script_type, const char* path, const char* full_path,
    const char* name, std::uint32_t context, std::uint32_t callback,
    IScriptInterpreterFactoryRuntime& runtime) {
    if (script_type != 0 && script_type != 1) {
        return nullptr;
    }

    void* memory = runtime.Allocate(script_type == 0 ? 28 : 40);
    if (!memory) {
        return nullptr;
    }

    ScriptInterpreterBaseState* interpreter;
    try {
        interpreter = script_type == 0 ? runtime.ConstructText(memory)
                                       : runtime.ConstructBinary(memory);
    } catch (...) {
        runtime.Free(memory);
        throw;
    }

    if (!ScriptInterpreterBase::SetPath(*interpreter, path, runtime) ||
        !ScriptInterpreterBase::SetFullPath(*interpreter, full_path, runtime) ||
        !ScriptInterpreterBase::SetName(*interpreter, name, runtime)) {
        runtime.Destroy(interpreter, 1);
        return nullptr;
    }

    if (!runtime.Initialize(interpreter, context, callback)) {
        runtime.Destroy(interpreter, 1);
        return nullptr;
    }
    return interpreter;
}

ScriptInterpreterBaseState* ScriptInterpreterFactory::Release(
    ScriptInterpreterBaseState* interpreter,
    IScriptInterpreterFactoryRuntime& runtime) {
    if (interpreter) {
        runtime.Destroy(interpreter, 1);
    }
    return interpreter;
}

}  // namespace pal4
