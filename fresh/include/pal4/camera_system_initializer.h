#pragma once

#include "pal4/camera_controller.h"
#include "pal4/camera_full_setup.h"

#include <array>
#include <cstdint>

namespace pal4 {

using CameraRenderRect = std::array<std::int32_t, 4>;

class ICameraSystemInitializerRuntime {
public:
    virtual ~ICameraSystemInitializerRuntime() = default;
    virtual void LogInitialization(bool complete) = 0;
    [[nodiscard]] virtual CameraRenderRect RenderRect() = 0;
    [[nodiscard]] virtual std::uint32_t AllocateObject(
        std::uint32_t bytes) = 0;
    virtual void ConstructObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool SetupObject(
        std::uint32_t object,
        const CameraSetupParameters& parameters) = 0;
    [[nodiscard]] virtual CameraDataState& CameraDataForObject(
        std::uint32_t object) = 0;
};

class CameraSystemInitializer final {
public:
    explicit CameraSystemInitializer(ICameraSystemInitializerRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5eb0a0: create four named cameras and seed controller parameters.
    [[nodiscard]] bool Initialize(CameraControllerState& state,
                                  std::uint32_t controller_address);

private:
    ICameraSystemInitializerRuntime& runtime_;
};

}  // namespace pal4
