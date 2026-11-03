#pragma once

#include "pal4/script_event_volume_visibility.h"

namespace pal4 {

class NativeSceneEvfAssets;
class NativeSceneMovementAsset;
class NativeCameraManagerOwner;

// Binds giEventVolumeVisible to the currently loaded EVF records.
class NativeScriptEventVolumeVisibilityRuntime final
    : public IScriptEventVolumeVisibilityRuntime {
public:
    NativeScriptEventVolumeVisibilityRuntime(
        NativeSceneEvfAssets& assets, NativeSceneMovementAsset& movement,
        std::uint32_t arena, NativeCameraManagerOwner* cameras = nullptr)
        : assets_(assets), movement_(movement), arena_(arena),
          cameras_(cameras) {}

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t EmptyStringAddress() override;
    [[nodiscard]] std::uint32_t EventVolumeManager() override;
    [[nodiscard]] std::uint32_t FindVolume(
        std::uint32_t manager, std::uint32_t name) override;
    [[nodiscard]] std::uint32_t ArenaManager() override;
    [[nodiscard]] std::uint32_t Deactivate(
        std::uint32_t arena, std::uint32_t volume) override;
    [[nodiscard]] std::uint32_t Activate(
        std::uint32_t arena, std::uint32_t volume) override;

private:
    NativeSceneEvfAssets& assets_;
    NativeSceneMovementAsset& movement_;
    std::uint32_t arena_;
    NativeCameraManagerOwner* cameras_;
};

}  // namespace pal4
