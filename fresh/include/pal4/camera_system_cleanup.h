#pragma once

#include "pal4/camera_controller.h"
#include "pal4/camera_property_cleanup.h"
#include "pal4/camera_system_object.h"

#include <cstdint>

namespace pal4 {

class CameraAllocation;

class ICameraSystemCleanupRuntime {
public:
    virtual ~ICameraSystemCleanupRuntime() = default;
    [[nodiscard]] virtual CameraSystemObjectState& ObjectAt(
        std::uint32_t address) = 0;
    virtual void FreeObject(std::uint32_t address) = 0;
};

class CameraSystemCleanup final {
public:
    CameraSystemCleanup(CameraAllocation& allocation,
                        CameraSystemObject& object,
                        CameraPropertyCleanup& properties,
                        ICameraSystemCleanupRuntime& runtime)
        : allocation_(allocation), object_(object),
          properties_(properties), runtime_(runtime) {}

    // 0x5eb540: release cameras, clean properties and reset the collection.
    [[nodiscard]] std::uint32_t Cleanup(CameraControllerState& state);

private:
    CameraAllocation& allocation_;
    CameraSystemObject& object_;
    CameraPropertyCleanup& properties_;
    ICameraSystemCleanupRuntime& runtime_;
};

}  // namespace pal4
