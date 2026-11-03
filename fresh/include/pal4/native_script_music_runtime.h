#pragma once

#include "pal4/script_music_transport.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace pal4 {

class NativeMilesMusicStream;

// Connects the original script music calls to a game-owned Miles stream.
class NativeScriptMusicRuntime final : public IScriptMusicTransportRuntime {
public:
    NativeScriptMusicRuntime(NativeMilesMusicStream& stream, std::string track);

    [[nodiscard]] bool Start();
    [[nodiscard]] std::uint32_t BackgroundMusicPlayer() override;
    [[nodiscard]] std::uint8_t StopSound(std::uint32_t sound_slot) override;
    [[nodiscard]] std::uint8_t LoadAndPlay(std::uint32_t player) override;

private:
    struct PlayerState {
        std::array<std::byte, 0x30> prefix{};
        std::uint32_t sound{};
    } player_;
    static_assert(offsetof(PlayerState, sound) == 0x30u);

    NativeMilesMusicStream& stream_;
    std::string track_;
};

}  // namespace pal4
