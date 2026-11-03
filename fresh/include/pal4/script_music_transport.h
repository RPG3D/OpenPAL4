#pragma once

#include <cstdint>

namespace pal4 {

class IScriptMusicTransportRuntime {
public:
    virtual ~IScriptMusicTransportRuntime() = default;
    [[nodiscard]] virtual std::uint32_t BackgroundMusicPlayer() = 0;
    [[nodiscard]] virtual std::uint8_t StopSound(
        std::uint32_t sound_slot) = 0;
    [[nodiscard]] virtual std::uint8_t LoadAndPlay(
        std::uint32_t player) = 0;
};

class ScriptMusicTransport final {
public:
    // IDA 0x5d8be0, giScriptMusicPause.
    [[nodiscard]] static std::uint8_t Pause(
        IScriptMusicTransportRuntime& runtime);
    // IDA 0x5d8bf0, giScriptMusicResume.
    [[nodiscard]] static std::uint8_t Resume(
        IScriptMusicTransportRuntime& runtime);
};

}  // namespace pal4
