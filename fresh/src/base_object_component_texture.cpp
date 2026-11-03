#include "pal4/base_object_component_texture.h"

#include <optional>

namespace pal4 {

std::uint8_t BaseObjectComponentTexture::SetTexture(
    BaseObjectState& state, std::uint32_t name_address,
    std::string_view name, IBaseObjectComponentTextureRuntime& runtime) {
    runtime.InitializeTemporary(static_cast<std::uint8_t>(name_address));
    (void)runtime.SetErrorFlag(0u);
    if (!runtime.AllocateAndCopy(name, 1u)) {
        return runtime.SetErrorFlag(1u);
    }

    std::optional<std::string> path;
    bool path_cleaned = false;
    bool temporary_released = false;
    const auto cleanup_path = [&] {
        path_cleaned = true;
        runtime.CleanupPath(*path);
    };
    const auto release_temporary = [&] {
        temporary_released = true;
        return runtime.ReleaseTemporary();
    };
    try {
        path.emplace(runtime.CreatePath(state.Read(8u)));
        path->append(name);
        path->append(".png");
        const auto dictionary = runtime.TextureDictionary();
        const auto texture = runtime.ResolveTexture(dictionary, *path);
        runtime.LoadTexture(state, name_address, texture);
        cleanup_path();
        return release_temporary();
    } catch (...) {
        if (path && !path_cleaned) cleanup_path();
        if (!temporary_released) (void)release_temporary();
        throw;
    }
}

}  // namespace pal4
