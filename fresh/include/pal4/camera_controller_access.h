#pragma once

#include "pal4/camera_controller.h"
#include "pal4/camera_data.h"
#include "pal4/camera_system_object.h"

#include <cstdint>
#include <optional>

namespace pal4 {

class ICameraControllerAccessRuntime {
public:
    virtual ~ICameraControllerAccessRuntime() = default;
    [[nodiscard]] virtual CameraSystemObjectState& ObjectAt(
        std::uint32_t address) = 0;
    virtual void SetFarClip(std::uint32_t camera, float distance) = 0;
    [[nodiscard]] virtual CameraDataState& CameraDataAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> FindNamedObject(
        const CameraControllerState& state, std::uint32_t collection_offset,
        const char* name) = 0;
    virtual void ReportMissingCameraName(const char* name) = 0;
};

class CameraControllerAccess final {
public:
    explicit CameraControllerAccess(ICameraControllerAccessRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5eb7d0: apply a far clip distance to the active camera.
    [[nodiscard]] bool SetFarClipPlane(const CameraControllerState& state,
                                       float distance);
    // 0x5eb800: resolve four built-in names, then the dynamic collection.
    [[nodiscard]] std::uint32_t GetObjectByName(
        const CameraControllerState& state, const char* name);
    // 0x5eb890: select a fixed or dynamically registered camera by name.
    [[nodiscard]] bool SetActiveCameraByName(
        CameraControllerState& state, const char* name);
    // 0x5eba10/0x5eba20/0x5eba30: unwrap active camera pointers.
    [[nodiscard]] std::uint32_t ActiveObject(
        const CameraControllerState& state) const;
    [[nodiscard]] std::uint32_t ActiveInternalId(
        const CameraControllerState& state);
    [[nodiscard]] std::uint32_t ActiveCameraPointer(
        const CameraControllerState& state);
    // 0x5eba40: true when the non-null Main camera is active.
    [[nodiscard]] bool IsMainCameraActive(
        const CameraControllerState& state) const;

private:
    ICameraControllerAccessRuntime& runtime_;
};

}  // namespace pal4
