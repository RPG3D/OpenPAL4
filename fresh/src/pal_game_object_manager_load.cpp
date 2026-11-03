#include "pal4/pal_game_object_manager_load.h"

#include <string>
#include <vector>

namespace pal4 {
namespace {

class StreamScope final {
public:
    StreamScope(IPalGameObjectManagerLoadRuntime& runtime,
                const std::uint32_t stream) : runtime_(runtime), stream_(stream) {}
    ~StreamScope() { runtime_.CloseStream(stream_); }

private:
    IPalGameObjectManagerLoadRuntime& runtime_;
    std::uint32_t stream_;
};

}  // namespace

bool PalGameObjectManagerLoad::LoadSceneObjects(
    const std::uint32_t manager, const std::string_view region,
    const std::string_view scene,
    IPalGameObjectManagerLoadRuntime& runtime) {
    std::string path(runtime.BasePath());
    path.append(region);
    path.append("\\");
    path.append(scene);
    path.append("\\GameObjs.gob");

    const auto stream = runtime.OpenStream(path, 2u, 1u);
    if (stream == 0u) return false;
    StreamScope stream_scope(runtime, stream);
    runtime.DeactivateAndClear(manager);

    std::uint32_t count = 0u;
    if (!runtime.ReadU32(stream, count)) return false;
    std::vector<std::uint32_t> types;
    for (std::uint32_t index = 0u; index < count; ++index) {
        std::uint32_t type = 0u;
        if (!runtime.ReadU32(stream, type)) return false;
        types.push_back(type);
    }

    for (const auto type : types) {
        const auto factory = runtime.FactoryForType(manager, type);
        if (factory == 0u) return false;
        const auto object = runtime.CreateObject(factory);
        if (object == 0u) return false;
        bool loaded = false;
        try {
            loaded = runtime.LoadRecord(object, stream);
            if (loaded)
                runtime.InsertObject(manager, runtime.ObjectName(object), object);
        } catch (...) {
            runtime.DestroyObject(object);
            throw;
        }
        if (!loaded) {
            runtime.DestroyObject(object);
            return false;
        }
    }
    return true;
}

}  // namespace pal4
