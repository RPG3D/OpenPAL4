#include "pal4/game_section_executor.h"

namespace pal4 {

void IGameSectionRuntime::InitializeScripts() {}
std::string IGameSectionRuntime::ArenaScriptPath() { return {}; }
std::uint32_t IGameSectionRuntime::ArenaScriptContext() { return 0u; }
std::uint32_t IGameSectionRuntime::CsbMode() { return 0u; }
bool IGameSectionRuntime::CacheContains(std::string_view) { return false; }
std::uint32_t IGameSectionRuntime::CreateInterpreter(
    std::uint32_t, std::string_view, std::string_view,
    std::string_view, std::uint32_t, std::uint32_t) { return 0u; }
bool IGameSectionRuntime::HandleExecutionException(std::uint32_t) {
    return false;
}
std::string IGameSectionRuntime::FunctionName(std::uint32_t) { return {}; }
std::string IGameSectionRuntime::ExceptionWhat(std::uint32_t) { return {}; }
void IGameSectionRuntime::ReportScriptException(
    std::string_view, std::string_view) {}
void IGameSectionRuntime::ReleaseException(std::uint32_t) {}
void IGameSectionRuntime::ShowMissingScript(
    std::string_view, std::string_view) {}
void IGameSectionRuntime::CleanupScripts() {}

std::uint32_t GameSectionExecutor::Execute(
    std::string_view section, IGameSectionRuntime& runtime) {
    section = section.substr(0u, section.find('\0'));
    runtime.InitializeScripts();
    const auto script_path = runtime.ArenaScriptPath();
    std::string cache_key = script_path;
    cache_key += '[';
    cache_key.append(section);
    cache_key += ']';

    if (!runtime.CacheContains(cache_key)) {
        const auto context = runtime.ArenaScriptContext();
        const auto mode = runtime.CsbMode();
        const auto interpreter = runtime.CreateInterpreter(
            mode, script_path, section, section, context,
            kCsbReadCallback);
        if (interpreter != 0u &&
            runtime.HandleExecutionException(interpreter)) {
            const auto function_name = runtime.FunctionName(interpreter);
            const auto what = runtime.ExceptionWhat(interpreter);
            runtime.ReportScriptException(what, function_name);
            runtime.ReleaseException(interpreter);
        } else {
            runtime.ShowMissingScript("not found script files", "no script");
        }
    }
    runtime.CleanupScripts();
    return 0u;
}

}  // namespace pal4
