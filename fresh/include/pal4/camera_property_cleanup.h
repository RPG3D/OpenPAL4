#pragma once

#include "pal4/camera_controller.h"

#include <cstdint>

namespace pal4 {

class ICameraPropertyCleanupRuntime {
public:
    virtual ~ICameraPropertyCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t IsEmpty(
        CameraControllerState& state, std::uint32_t offset) = 0;
    virtual void ClearCollection(CameraControllerState& state,
                                 std::uint32_t offset) = 0;
};

class CameraPropertyCleanup final {
public:
    explicit CameraPropertyCleanup(ICameraPropertyCleanupRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5eb600: reset active camera to the first slot after collection cleanup.
    [[nodiscard]] std::uint32_t Cleanup(CameraControllerState& state);

private:
    ICameraPropertyCleanupRuntime& runtime_;
};

}  // namespace pal4
