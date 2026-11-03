#pragma once
#include <array>
#include <cstdint>
#include <functional>

namespace pal4 {
class NativeGameArenaManagerOwner;
class NativeSceneGobAssets;
class NativeSceneMovementAsset;
class NativeCameraManagerOwner;
class NativeGobObjectImage;
struct NativeGameWorldFadeServices {
    std::function<NativeGobObjectImage*(std::uint32_t)> resolve_gob;
    std::function<void()> refresh_minimap;
    std::function<void(std::uint32_t)> leave_character;
};
struct NativeGameWorldFadeResult {
    // Original order: fade-out620, fade-in636, character fade-out652.
    std::array<std::uint32_t,3> before{},after{};
    std::uint32_t material_updates{},removed_gobs{},removed_characters{};
};
// Borrows the arena's original three record arrays; no replacement timers,
// immediate removal or extra SDK clump addition. Caller advances once per
// original world update, with all record targets/creating owners still alive.
class NativeGameWorldFadeQueues final {
public:
    static NativeGameWorldFadeResult Advance(NativeGameArenaManagerOwner&,
        NativeSceneMovementAsset&,NativeCameraManagerOwner*,float elapsed,
        NativeGameWorldFadeServices);
    static NativeGameWorldFadeResult Advance(NativeGameArenaManagerOwner&,
        NativeSceneGobAssets&,NativeSceneMovementAsset&,NativeCameraManagerOwner*,
        float elapsed,std::function<void()> refresh_minimap,
        std::function<void(std::uint32_t)> leave_character={});
};
}
