#pragma once

#include <cstdint>

namespace pal4 {

class IPlayerManagerSlotCameraAudioRuntime {
public:
    virtual ~IPlayerManagerSlotCameraAudioRuntime() = default;
    virtual void SetBaseProperty(std::uint32_t object,
                                 std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t manager) = 0;
    virtual void SetLookAt(std::uint32_t camera, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraInternal(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual std::uint32_t SetListenerPosition(
        std::uint32_t listener, float x, float y, float z) = 0;
    [[nodiscard]] virtual std::uint32_t SetListenerOrientation(
        std::uint32_t listener, float x, float y, float z,
        float up_x, float up_y, float up_z) = 0;
};

class PlayerManagerSlotCameraAudioRuntimeStub :
    public IPlayerManagerSlotCameraAudioRuntime {
public:
    void SetBaseProperty(std::uint32_t, std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint32_t CameraManager() override;
    std::uint32_t ActiveCameraObject(std::uint32_t) override;
    void SetLookAt(std::uint32_t, std::uint32_t) override;
    std::uint32_t ActiveCameraInternal(std::uint32_t) override;
    std::uint32_t AudioManager() override;
    std::uint32_t SetListenerPosition(std::uint32_t, float, float,
                                      float) override;
    std::uint32_t SetListenerOrientation(std::uint32_t, float, float,
                                         float, float, float,
                                         float) override;
};

class PlayerManagerSlotCameraAudio final {
public:
    // IDA 0x43edd0: set property and synchronize active camera to 3D audio.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object, std::uint32_t property,
        IPlayerManagerSlotCameraAudioRuntime& runtime);
};

}  // namespace pal4
