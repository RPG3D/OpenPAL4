#include "pal4/minimap_sprite_loader.h"

namespace pal4 {

std::uint32_t MinimapSpriteLoader::InitializeThunk(
    IMinimapSpriteRuntime& runtime) {
    return InitializeAndLoad(runtime);
}

namespace {

std::string_view IconForType(const std::uint32_t type) {
    switch (type) {
    case 2u: return "Grocer.PNG";
    case 3u: return "Entry.PNG";
    case 5u: return "Drugstore.png";
    case 6u: return "Forger.png";
    case 7u: return "Inn.png";
    case 8u: return "Remind.png";
    case 9u: return "Restaurant.png";
    case 10u: return "SaveSpot.png";
    default: return {};
    }
}

}  // namespace

std::uint32_t MinimapSpriteLoader::InitializeAndLoad(
    IMinimapSpriteRuntime& runtime) {
    runtime.CleanupSection(runtime.GetMinimap(), 1u);
    const auto manager = runtime.GetObjectManager();
    return runtime.IterateObjects(manager,
        [&runtime](const std::uint32_t object) {
            return LoadObjectIcon(object, runtime);
        }, 0u);
}

std::uint32_t MinimapSpriteLoader::LoadObjectIcon(
    const std::uint32_t object, IMinimapSpriteRuntime& runtime) {
    if (runtime.ReadDword(object + 432u) == 0u ||
        runtime.ReadDword(object + 388u) == 0u ||
        runtime.ReadDword(object + 408u) == 0u) {
        return object;
    }
    const auto position = runtime.ReadPosition(object + 172u);
    const auto coordinates = runtime.ConvertGridCoordinates(
        runtime.GetMinimap(), position);
    const auto type = runtime.ReadDword(object + 432u);
    const auto icon = IconForType(type);
    if (!icon.empty()) {
        const auto minimap = runtime.GetMinimap();
        const auto y = runtime.ConvertFloatToInt(coordinates[1]);
        const auto x = runtime.ConvertFloatToInt(coordinates[0]);
        runtime.AddIcon(minimap, runtime.ReadDword(object + 4u),
                        1u, icon, x, y);
    }
    return object;
}

}  // namespace pal4
