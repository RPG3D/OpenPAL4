#include "pal4/script_execution_context.h"

#include <string>

namespace pal4 {

namespace {

constexpr char kMissingModule[] =
    "error<executor::addScriptModule>: add a null script-module\r\n";
constexpr char kMissingEngine[] =
    "error<executor::addScriptModule>: null scriptEngine\r\n";
constexpr char kEmptyName[] =
    "error<executor:addScriptModule>: null scriptFunc\r\n";
constexpr char kLoadBytecode[] =
    "<executor::addScriptModule>: load byte module ";
constexpr char kAddSection[] =
    "<executor::addScriptModule>: add script section ";
constexpr char kBuild[] =
    "<executor::addScriptModule>: build script module ";

bool InvalidInput(std::uint32_t logger, const char* message,
                  IScriptModuleBoundary& runtime) {
    if (logger != 0) {
        runtime.ReportRaw(logger,
                          reinterpret_cast<std::uintptr_t>(message));
    } else {
        runtime.RaiseError(message);
    }
    return false;
}

void LogModuleHeader(std::uint32_t logger, const char* path,
                     IScriptModuleBoundary& runtime) {
    if (logger == 0) return;
    std::string text = "\r\n\r\n--<<<< ";
    text += path;
    text += " >>>>--\r\n";
    runtime.EmitLog(logger, text);
}

}  // namespace

bool ScriptExecutionContext::AddScriptModule(
    const ScriptExecutionContextState& state, void* interpreter,
    IScriptModuleBoundary& runtime) {
    if (interpreter == nullptr) {
        return InvalidInput(state.word_8, kMissingModule, runtime);
    }
    if (state.script_engine == nullptr) {
        return InvalidInput(state.word_8, kMissingEngine, runtime);
    }

    const char* name = runtime.Name(interpreter);
    if (name[0] == '\0') {
        if (state.word_8 == 0) {
            runtime.RaiseError(kEmptyName);
            return false;
        }
        // The EXE really pushes 0x90909090 here; the native callback may fault.
        runtime.ReportRaw(state.word_8, 0x90909090u);
    }

    const int kind = runtime.Kind(interpreter);
    if (kind != 0 && kind != 1) return false;

    const char* path = runtime.FullPath(interpreter);
    LogModuleHeader(state.word_8, path, runtime);
    if (kind == 1) {
        const auto bytecode = runtime.Bytecode(interpreter);
        const char* load_path = runtime.FullPath(interpreter);
        const int result = runtime.LoadBytecode(
            state.script_engine, load_path, bytecode);
        runtime.CheckResult(result, kLoadBytecode, state.word_8);
        return true;
    }

    const auto source = runtime.Source(interpreter);
    const char* section = runtime.SectionPath(interpreter);
    const char* section_module_path = runtime.FullPath(interpreter);
    const int add_result = runtime.AddSection(
        state.script_engine, section_module_path, section, source);
    runtime.CheckResult(add_result, kAddSection, state.word_8);
    const char* build_path = runtime.FullPath(interpreter);
    const int build_result = runtime.Build(
        state.script_engine, build_path, state.word_8);
    runtime.CheckResult(build_result, kBuild, state.word_8);
    return build_result != -1;
}

}  // namespace pal4
