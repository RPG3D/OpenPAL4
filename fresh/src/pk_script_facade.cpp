#include "pal4/pk_script_facade.h"

namespace pal4 {
namespace {

std::uint32_t ScriptManager(IPkScriptFacadeRuntime& runtime) {
    const auto manager = runtime.InitializeManager();
    return runtime.GetScriptManager(manager);
}

}  // namespace

std::uint32_t PkScriptFacade::LoadByName(
    const std::uint32_t, const std::uint32_t key,
    const char* path, IPkScriptFacadeRuntime& runtime) {
    const auto scripts = ScriptManager(runtime);
    if (scripts == 0) return 3u;
    const auto file = PackageResourceManager::OpenFile(
        path, runtime.Resources());
    if (file == 0) return 3u;
    (void)ScriptManagerCore::InsertScript(
        scripts, key, file, runtime.Scripts());
    return 1u;
}

std::uint32_t PkScriptFacade::Unload(
    const std::uint32_t key, IPkScriptFacadeRuntime& runtime) {
    const auto scripts = ScriptManager(runtime);
    if (scripts == 0) return 0u;
    if (ScriptManagerCore::CloseScript(
            scripts, key, runtime.Scripts()) == 0)
        return 0u;
    return ScriptManagerCore::RemoveScript(
        scripts, key, runtime.Scripts());
}

std::uint32_t PkScriptFacade::Read(
    const std::uint32_t key, void* buffer,
    const std::uint32_t bytes, IPkScriptFacadeRuntime& runtime) {
    const auto scripts = ScriptManager(runtime);
    return scripts == 0 ? 0u : ScriptManagerCore::ReadScript(
        scripts, key, buffer, bytes, runtime.Scripts());
}

bool PkScriptFacade::IsLoaded(
    const std::uint32_t key, IPkScriptFacadeRuntime& runtime) {
    const auto scripts = ScriptManager(runtime);
    return scripts != 0 && ScriptManagerCore::IsScriptLoaded(
        scripts, key, runtime.Scripts());
}

std::uint64_t PkScriptFacade::Seek(
    const std::uint32_t key, const std::int32_t offset,
    const std::uint32_t mode, IPkScriptFacadeRuntime& runtime) {
    const auto scripts = ScriptManager(runtime);
    if (scripts == 0 || mode < 1u || mode > 3u) return 0u;
    const auto result = ScriptManagerCore::SeekScript(
        scripts, key, offset, static_cast<std::int32_t>(mode - 1u),
        runtime.Scripts());
    return static_cast<std::uint32_t>(result);
}

std::uint8_t PkScriptFacade::FileExists(
    const std::uint32_t, const char* path,
    IPkScriptFacadeRuntime& runtime) {
    const auto scripts = ScriptManager(runtime);
    return scripts == 0 ? 0u : PackageResourceManager::FileExists(
        path, runtime.Resources());
}

}  // namespace pal4
