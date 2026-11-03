#pragma once

#include "pal4/script_gameplay_callbacks.h"

#include <cstddef>

namespace pal4 {

class NativeActorControlSession;
class NativeCameraManagerOwner;
class NativePlayerManagerSession;

class NativeScriptCameraModeRuntime final : public IScriptCameraModeRuntime {
public:
    NativeScriptCameraModeRuntime(NativeCameraManagerOwner& cameras,
                                  NativeActorControlSession& control,
                                  NativePlayerManagerSession& players);

    std::uint32_t CameraSubsystem() override;
    void SetModeByIndex(std::uint32_t manager, std::uint32_t mode) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t ActiveCameraInternal(std::uint32_t manager) override;
    void SetPosition(std::uint32_t camera, std::uint32_t x,
                     std::uint32_t y, std::uint32_t z,
                     std::uint32_t keep_look_at) override;
    std::uint32_t ActiveCameraObject(std::uint32_t manager) override;
    void SetLookAt(std::uint32_t camera, std::uint32_t flag) override;
    std::uint32_t ActorControl() override;
    void FlushMainCameraTailYaw(std::uint32_t actor) override;
    std::uint32_t PlayerManager() override;
    std::int32_t SetPlayerState(std::uint32_t manager,
                                std::uint8_t sync) override;

private:
    std::size_t CameraIndex(std::uint32_t object) const;
    std::uint32_t InternalCamera(std::uint32_t object) const;

    NativeCameraManagerOwner& cameras_;
    NativeActorControlSession& control_;
    NativePlayerManagerSession& players_;
};

}  // namespace pal4
