#include "pal4/game_data_file_loader.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kDatabaseVersion = 20060228;

class LoadResources final {
public:
    explicit LoadResources(IGameDataFileLoaderRuntime& runtime)
        : runtime_(runtime) {}
    ~LoadResources() {
        if (stream) runtime_.DestroyStream(stream);
        if (file) runtime_.CloseFile(file);
        if (script) {
            runtime_.CleanupAllScripts();
            runtime_.DestroyBootstrapScript(script);
        }
    }
    LoadResources(const LoadResources&) = delete;
    LoadResources& operator=(const LoadResources&) = delete;
    std::uint32_t script = 0;
    std::uint32_t file = 0;
    std::uint32_t stream = 0;

private:
    IGameDataFileLoaderRuntime& runtime_;
};

}  // namespace

bool GameDataFileLoader::Load(
    const char* path, std::uint32_t database, std::uint32_t context,
    IGameDataFileLoaderRuntime& runtime) {
    LoadResources resources(runtime);
    if (runtime.LoadBootstrapEnabled()) {
        resources.script = runtime.LoadBootstrapScript(
            "gamedata\\database.cpk");
        if (resources.script)
            runtime.ProcessBootstrapScript(resources.script);
    }
    if (!path) return false;
    resources.file = runtime.OpenFile(path, "rb");
    if (!resources.file) return false;
    resources.stream = runtime.CreateStream(resources.file);

    if (runtime.ReadText(resources.stream) != runtime.ExpectedSignature())
        return false;
    if (runtime.ReadWord(resources.stream) != kDatabaseVersion)
        return false;

    const auto script_count = runtime.ReadWord(resources.stream);
    for (std::uint32_t i = 0; i < script_count; ++i) {
        const auto name = runtime.ReadText(resources.stream);
        if (!runtime.LoadScriptEntry(name, database, resources.stream,
                                     context))
            return false;
    }

    const auto table_count = runtime.ReadWord(resources.stream);
    for (std::uint32_t i = 0; i < table_count; ++i) {
        const auto name = runtime.ReadText(resources.stream);
        if (!runtime.LoadTableEntry(name, database, resources.stream,
                                    context)) {
            runtime.RemoveTableEntry(database, name);
            return false;
        }
    }
    return true;
}

}  // namespace pal4
