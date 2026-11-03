#include "pal4/thunder_track.h"

namespace pal4 {

std::uint32_t ThunderTrack::SetTargetPosition(
    ThunderObjectState& state, const std::array<std::uint32_t, 3u>& values,
    const std::uint32_t entry_eax, IThunderTrackRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return entry_eax & 0xffffff00u;

    if ((state.words[14u] & 0xffu) != 0u) {
        for (std::uint32_t i = 0u; i < 3u; ++i)
            state.words[37u + i] = values[i];
        return (entry_eax & 0xffffff00u) | (values[1u] & 0xffu);
    }

    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[24u + i] = values[i];
    return runtime.UpdateFrame(state);
}

}  // namespace pal4
