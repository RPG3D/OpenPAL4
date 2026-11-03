#pragma once

#include <cstdint>

namespace pal4 {

class NativeSceneEvfAssets;
class NativeSceneMovementAsset;
class NativeCameraManagerOwner;

class NativeArenaEvfSceneEntry final {
public:
    [[nodiscard]] static std::uint32_t EnterOne(
        std::uint32_t record, std::uint32_t arena,
        NativeSceneMovementAsset& movement,
        NativeCameraManagerOwner* cameras = nullptr);
    [[nodiscard]] static std::uint32_t LeaveOne(
        std::uint32_t record, std::uint32_t arena,
        NativeSceneMovementAsset& movement,
        NativeCameraManagerOwner* cameras = nullptr);
    [[nodiscard]] static std::uint32_t EnterAll(
        NativeSceneEvfAssets& assets, std::uint32_t arena,
        NativeSceneMovementAsset& movement,
        NativeCameraManagerOwner* cameras = nullptr);
    static void LeaveAll(NativeSceneEvfAssets& assets, std::uint32_t arena,
                         NativeSceneMovementAsset& movement,
                         NativeCameraManagerOwner* cameras = nullptr);
};

}  // namespace pal4
