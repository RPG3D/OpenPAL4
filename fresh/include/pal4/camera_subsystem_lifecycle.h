#pragma once

#include "pal4/camera_controller.h"

#include <cstdint>
#include <optional>

namespace pal4 {

struct CameraSubsystemRegistrationState {
    std::uint8_t guard{};  // 0x90f128
    std::uint32_t instance{};  // 0x90f124
};

class ICameraSubsystemRuntime {
public:
    virtual ~ICameraSubsystemRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, const char* name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t cleanup) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual CameraControllerState& ControllerAt(
        std::uint32_t memory) = 0;
    virtual void CleanupCameraObject(std::uint32_t camera_object) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t memory) = 0;
};

class CameraSubsystemLifecycle final {
public:
    CameraSubsystemLifecycle(ICameraSubsystemRuntime& runtime,
                             CameraController& controller)
        : runtime_(runtime), controller_(controller) {}

    // 0x5eade0: register palCameraManager once with singleton ID 1002.
    [[nodiscard]] std::uint32_t Register(
        CameraSubsystemRegistrationState& state);
    // 0x5eae20: allocate and construct the 0xc4-byte controller.
    [[nodiscard]] std::uint32_t Create();
    // 0x5eae80: clean up and free a non-null controller.
    [[nodiscard]] std::optional<std::uint32_t> Destroy(
        std::uint32_t instance);
    // 0x5eaea0: destroy a camera object and free its outer allocation.
    [[nodiscard]] std::optional<std::uint32_t> DestroyCameraObject(
        std::uint32_t camera_object);

private:
    ICameraSubsystemRuntime& runtime_;
    CameraController& controller_;
};

}  // namespace pal4
