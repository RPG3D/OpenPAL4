#pragma once

#include <cstdint>
#include <string_view>
#include "pal4/script_camera_query.h"

namespace pal4 {

struct PlayerManagerObjectState;

class IScriptCameraCallbackRuntime : public IScriptCameraQueryRuntime {
public:
    std::uint32_t InitializeCameraSubsystem() final { return CameraSubsystem(); }
    float CameraFloat(std::uint32_t address) final { return ReadFloat(address); }
    virtual ~IScriptCameraCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t CameraByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CameraInternalId(
        std::uint32_t camera) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t manager) = 0;
    // 0x5d3000: incremental control of the current active camera, not MainCamera.
    virtual void AdjustYpr(float yaw, float pitch, float roll, std::uint8_t option) = 0;
    virtual void SetLookAt(std::uint32_t object, std::uint32_t option) = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraInternalId(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetListenerPosition(std::uint32_t handle, float x,
                                     float y, float z) = 0;
    [[nodiscard]] virtual std::uint32_t SetListenerOrientation(
        std::uint32_t handle, float at_x, float at_y, float at_z,
        float up_x, float up_y, float up_z) = 0;
    [[nodiscard]] virtual PlayerManagerObjectState& PlayerManagerState() = 0;
};

class ScriptCameraCallbacks final {
public:
    // IDA 0x5d4dd0, giCameraSetCollide.
    static void SetCollision(std::uint8_t enabled,
                             IScriptCameraCallbackRuntime& runtime);
    // IDA 0x5d4e40, giCameraSeekToPlayer.
    [[nodiscard]] static std::uint32_t SeekToPlayer(
        IScriptCameraCallbackRuntime& runtime);
    // IDA 0x5d4ef0, giCameraAutoSeek.
    [[nodiscard]] static std::uint32_t AutoSeek(
        std::uint32_t enabled, IScriptCameraCallbackRuntime& runtime);
};

}  // namespace pal4
