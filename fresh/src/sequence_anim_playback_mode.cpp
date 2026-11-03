#include "pal4/sequence_anim_playback_mode.h"

#include <bit>

namespace pal4 {

void SequenceAnimPlaybackMode::Set(
    SequenceAnimObjectState& state, const std::int32_t mode,
    ISequenceAnimPlaybackModeRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return;
    const auto mode_byte = static_cast<std::uint8_t>(mode);
    if (static_cast<std::uint8_t>(state.words[2u] >> 24u) == mode_byte)
        return;
    state.words[2u] = (state.words[2u] & 0x00ffffffu) |
                      (static_cast<std::uint32_t>(mode_byte) << 24u);
    if (mode_byte == 0u)
        return;

    std::int32_t first = 0;
    std::int32_t second = 0;
    std::int32_t third = 0;
    std::int32_t frame = 0;
    runtime.QueryPlayback(state.words[9u], 36u, first, second, third, frame);
    const auto ratio = static_cast<float>(frame) / runtime.Duration(state);
    state.words[29u] = std::bit_cast<std::uint32_t>(static_cast<float>(ratio));
}

}  // namespace pal4
