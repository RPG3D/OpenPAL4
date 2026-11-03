#include "pal4/gob_normal.h"
namespace pal4 {
std::uint32_t GobNormal::RunScriptAndConsumeUse(
    const std::uint32_t object, IGobNormalScriptRuntime& runtime) {
    const auto use_count = runtime.ReadWord(object + 0x198u);
    if (use_count == 0u) return 0u;
    const auto length = static_cast<std::int32_t>(
        runtime.ReadWord(object + 0x17cu));
    if (length > 0) {
        runtime.InitializeScript();
        const auto path = runtime.CurrentSceneScriptPath();
        const auto entry = runtime.ReadCString(
            runtime.ReadWord(object + 0x178u)).substr(0, length);
        const auto key = path + "[" + std::string(entry) + "]";
        if (runtime.ScriptCacheContains(key)) {
            runtime.CleanupScriptPackages();
            return runtime.CleanupScriptPath();
        }
        const auto interpreter = runtime.CreateScriptInterpreter(
            path, key, entry);
        if (interpreter != 0u &&
            runtime.AddScriptModule(interpreter)) {
            runtime.PushStackModule(interpreter);
            runtime.ReleaseInterpreter(interpreter);
        } else {
            runtime.ShowScriptNotFound();
        }
        runtime.CleanupScriptPackages();
        static_cast<void>(runtime.CleanupScriptPath());
    }
    return runtime.ConsumeResearchUse(object);
}

}
