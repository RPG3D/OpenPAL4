#include "pal4/game_arena_config_deserializer.h"

#include <string>

namespace pal4 {
namespace {

std::string CopyCString(const std::string_view text) {
    return std::string(text.substr(0, text.find('\0')));
}

}  // namespace

bool GameArenaConfigDeserializer::Deserialize(
    const std::uint32_t arena, const std::uint32_t source,
    IGameArenaConfigDeserializeRuntime& runtime) {
    runtime.WriteWord(arena + 196u, 0u);
    runtime.WriteWord(arena + 444u, 1u);

    const auto marker = runtime.ReadSerializedMarker(source);
    if (marker != "ARENA_SERIALIZE_FLAG") {
        runtime.ReleaseInput();
        return false;
    }

    const auto node = runtime.CreateNode("ARENA");
    if (!runtime.CheckVersionAndType(source, node)) {
        runtime.DestroyNode(node);
        runtime.ReleaseInput();
        return false;
    }

    const auto world = CopyCString(runtime.ReadAttribute(node, "world", ""));
    const auto area = CopyCString(runtime.ReadAttribute(node, "area", ""));
    const auto diff = CopyCString(runtime.ReadAttribute(node, "diff", ""));
    runtime.LoadSceneResources(arena, world, area, 1u);
    runtime.LoadSceneRuntime(arena, diff);
    runtime.InitializePlayerScene(arena);
    if (runtime.ReadWord(arena + 236u) == 0u) {
        runtime.WriteWord(arena + 196u, 1u);
        runtime.WriteWord(arena + 444u, 0u);
    }

    runtime.ReleaseAttribute("diff");
    runtime.ReleaseAttribute("area");
    runtime.ReleaseAttribute("world");
    runtime.DestroyNode(node);
    runtime.ReleaseInput();
    return true;
}

}  // namespace pal4
