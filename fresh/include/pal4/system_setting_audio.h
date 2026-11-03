#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISystemSettingAudioRuntime {
public:
    virtual ~ISystemSettingAudioRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void SetStreamVolume(std::uint32_t stream, float value) = 0;
    [[nodiscard]] virtual std::uint32_t StreamType(std::uint32_t stream) = 0;
    virtual void RefreshStream(std::uint32_t stream) = 0;
    virtual void PlayMusic(std::string_view resource,
                           std::uint32_t mode) = 0;
};

class SystemSettingAudio final {
public:
    [[nodiscard]] static std::uint8_t OnBgmVolumeChanged(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingAudioRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnSfxVolumeChanged(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingAudioRuntime& runtime);
    [[nodiscard]] static std::uint8_t PlayButtonSound(
        std::uint32_t event, ISystemSettingAudioRuntime& runtime);
};

}  // namespace pal4
