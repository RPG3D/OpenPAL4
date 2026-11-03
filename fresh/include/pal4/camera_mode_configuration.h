#pragma once

#include "pal4/camera_controller.h"
#include "pal4/camera_data.h"
#include "pal4/camera_system_object.h"

#include <cstdint>

namespace pal4 {

class CameraParameterSetters;
class CameraControllerAccess;

class ICameraModeConfigurationRuntime {
public:
    virtual ~ICameraModeConfigurationRuntime() = default;
    [[nodiscard]] virtual CameraSystemObjectState& ObjectAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual CameraDataState& CameraDataAt(
        std::uint32_t address) = 0;
    virtual void ClearObjectCollection(std::uint32_t object,
                                       std::uint32_t offset) = 0;
};

class CameraModeConfiguration final {
public:
    CameraModeConfiguration(CameraParameterSetters& parameters,
                            CameraControllerAccess& access,
                            ICameraModeConfigurationRuntime& runtime)
        : parameters_(parameters), access_(access), runtime_(runtime) {}

    // 0x5eb630: configure main and script cameras from the controller table.
    [[nodiscard]] std::uint32_t Configure(
        CameraControllerState& state, std::uint32_t controller_address);
    // 0x5eb960: apply one of six camera parameter-table entries.
    [[nodiscard]] bool SetModeByIndex(CameraControllerState& state,
                                      std::uint32_t controller_address,
                                      std::uint32_t index);

private:
    CameraParameterSetters& parameters_;
    CameraControllerAccess& access_;
    ICameraModeConfigurationRuntime& runtime_;
};

}  // namespace pal4
