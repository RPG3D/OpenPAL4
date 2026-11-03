#include "pal4/game_arena_lightmap_config_loader.h"

#include <string>

namespace pal4 {
namespace {

std::string CopyCString(const std::string_view text) {
    return std::string(text.substr(0, text.find('\0')));
}

}  // namespace

bool GameArenaLightmapConfigLoader::Load(
    const std::uint32_t arena, IGameArenaLightmapConfigRuntime& runtime) {
    const auto directory = CopyCString(runtime.ReadCString(arena + 4u));
    const auto scene = CopyCString(runtime.ReadCString(arena + 12u));
    const std::string path = directory + scene + "_ltMap.cfg";
    const auto file = runtime.Open(path, "rb");
    if (file == 0u) {
        runtime.ReleasePath(path);
        return false;
    }

    runtime.Read(arena + 0x1acu, 12u, 1u, file);
    runtime.Read(arena + 0x1b8u, 4u, 1u, file);
    runtime.Close(file);
    runtime.ReleasePath(path);
    return true;
}

}  // namespace pal4
