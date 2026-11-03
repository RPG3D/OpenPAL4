#pragma once

#include "pal4/script_camera_callbacks.h"

#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {

class NativeCameraManagerOwner;
class NativePlayerManagerSession;

class INativeScriptCameraListener {
public:
    virtual ~INativeScriptCameraListener() = default;
    [[nodiscard]] virtual std::uint32_t Address() const = 0;
    [[nodiscard]] virtual std::uint32_t Handle() const = 0;
    virtual void SetPosition(float x, float y, float z) = 0;
    [[nodiscard]] virtual std::uint32_t SetOrientation(
        float at_x, float at_y, float at_z,
        float up_x, float up_y, float up_z) = 0;
};

// Adapts the owned camera and player images to the scene script callbacks.
class NativeScriptCameraRuntime final : public IScriptCameraCallbackRuntime {
public:
    NativeScriptCameraRuntime(NativeCameraManagerOwner& cameras,
                              NativePlayerManagerSession& players);
    void BindLookAt(std::function<void(std::uint32_t, std::uint32_t)> update);
    void BindListener(INativeScriptCameraListener& listener) noexcept;

    std::uint32_t CameraSubsystem() override;
    std::uint32_t CameraByName(std::uint32_t manager,
                               std::string_view name) override;
    std::uint32_t CameraInternalId(std::uint32_t camera) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t ActiveCameraObject(std::uint32_t manager) override;
    void AdjustYpr(float yaw, float pitch, float roll, std::uint8_t option) override;
    void SetLookAt(std::uint32_t object, std::uint32_t option) override;
    std::uint32_t ActiveCameraInternalId(std::uint32_t manager) override;
    float ReadFloat(std::uint32_t address) override;
    std::uint32_t AudioManager() override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void SetListenerPosition(std::uint32_t handle, float x, float y,
                             float z) override;
    std::uint32_t SetListenerOrientation(std::uint32_t handle, float at_x,
                                         float at_y, float at_z, float up_x,
                                         float up_y, float up_z) override;
    PlayerManagerObjectState& PlayerManagerState() override;

private:
    NativeCameraManagerOwner& cameras_;
    NativePlayerManagerSession& players_;
    std::function<void(std::uint32_t, std::uint32_t)> look_at_;
    INativeScriptCameraListener* listener_{};
};

}  // namespace pal4
