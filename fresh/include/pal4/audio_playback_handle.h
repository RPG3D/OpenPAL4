#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IAudioPlaybackHandleRuntime {
public:
    virtual ~IAudioPlaybackHandleRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

struct AudioPathState {
    std::uint32_t data{};
    std::uint32_t length{};
    std::uint32_t capacity{};
};

class IPlaybackMusicRuntime : public IAudioPlaybackHandleRuntime {
public:
    [[nodiscard]] virtual std::uint8_t IsPlaying(
        std::uint32_t handle_address) = 0;
    virtual void InitializePath(AudioPathState& path,
                                std::uint8_t flag,
                                std::string_view prefix) = 0;
    [[nodiscard]] virtual bool AllocateText(AudioPathState& path,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyString() = 0;
    [[nodiscard]] virtual std::uint32_t Play(
        std::uint8_t flag, std::uint32_t path,
        std::uint32_t repeat) = 0;
    [[nodiscard]] virtual std::uint8_t Free(
        std::uint32_t address) = 0;
};

class AudioPlaybackHandle final {
public:
    static constexpr std::uint32_t kAddress = 0x8edf80u;

    static void ResetThunk(IAudioPlaybackHandleRuntime& runtime);
    static void Reset(IAudioPlaybackHandleRuntime& runtime);
    [[nodiscard]] static std::uint8_t PlayMusic(
        std::string_view file, std::uint8_t flag,
        IPlaybackMusicRuntime& runtime);
};

}  // namespace pal4
