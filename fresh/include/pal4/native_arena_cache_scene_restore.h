#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {
class NativeArenaCacheOwner;
class NativeSceneGobAssets;
class NativeMinimapManagerOwner;
class NativeMonsterManagerOwner;

class NativeArenaCacheSceneRestore final {
public:
    // Installs scene callbacks for this synchronous restore only, then removes
    // them on success or exception. Requires exclusive callback ownership.
    // Scene changes are incremental. A monster manager is required when the
    // matched cache contains monster snapshots.
    [[nodiscard]] static std::uint32_t Restore(
        NativeArenaCacheOwner& cache, std::string_view world,
        std::string_view scene, NativeSceneGobAssets& objects,
        NativeMinimapManagerOwner& minimap, NativeMonsterManagerOwner* monsters = nullptr);
};
}  // namespace pal4
