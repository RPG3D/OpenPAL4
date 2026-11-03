#include "pal4/game_arena_config_serializer.h"

#include <array>
#include <string>

namespace pal4 {
namespace {

std::string CopyCString(const std::string_view text) {
    return std::string(text.substr(0, text.find('\0')));
}

}  // namespace

std::uint8_t GameArenaConfigSerializer::Serialize(
    const std::uint32_t arena, const std::uint32_t target,
    IGameArenaConfigRuntime& runtime) {
    constexpr std::string_view marker = "ARENA_SERIALIZE_FLAG";
    runtime.PrepareTarget(target, marker);
    const auto node = runtime.CreateNode("ARENA");
    constexpr std::array<std::uint32_t, 3> offsets{8u, 12u, 224u};
    constexpr std::array<std::string_view, 3> names{"world", "area", "diff"};

    for (std::size_t index = 0; index < names.size(); ++index) {
        const auto value = CopyCString(runtime.ReadCString(arena + offsets[index]));
        if (runtime.HasAttribute(node, names[index])) {
            runtime.SetAttribute(node, names[index], value);
        } else {
            runtime.AddAttribute(node, names[index], value);
        }
    }

    const auto result = runtime.Write(node, target);
    runtime.DestroyNode(node);
    runtime.ReleaseMarker(marker);
    return result;
}

}  // namespace pal4
