#pragma once

#include "pal4/audio_playback_handle.h"
#include <vector>

namespace pal4 {
class NativeMilesSound2dRuntime;

// Owns the game's shared UI sound handle; Miles owns samples through sounds.
// Reuses IDA4AC280 policy: flag gates an already playing sound, repeat is one.
class NativeUiSoundPlayback final : private IPlaybackMusicRuntime {
public:
    explicit NativeUiSoundPlayback(NativeMilesSound2dRuntime& sounds) : sounds_(sounds) {}
    void Play(std::string_view file, std::uint8_t flag);
    [[nodiscard]] std::uint32_t Handle() const noexcept { return handle_; }
private:
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t IsPlaying(std::uint32_t address) override;
    void InitializePath(AudioPathState& path, std::uint8_t flag, std::string_view prefix) override;
    bool AllocateText(AudioPathState& path, std::uint32_t length, std::uint8_t mode) override;
    void CopyText(std::uint32_t destination, std::string_view text) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    std::uint32_t EmptyString() override;
    std::uint32_t Play(std::uint8_t flag, std::uint32_t path, std::uint32_t repeat) override;
    std::uint8_t Free(std::uint32_t address) override;
    char* Resolve(std::uint32_t address, std::size_t size);
    NativeMilesSound2dRuntime& sounds_;
    std::uint32_t handle_{};
    // Host-owned temporary text exposes only the boundary required by the
    // existing game helper. std::vector supplies allocation and preservation.
    std::vector<char> path_;
    char empty_{};
};
}  // namespace pal4
