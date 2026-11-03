#include "pal4/thunder_visual_storage.h"

#include <bit>
#include <cmath>
#include <cstdint>

namespace pal4 {

std::uint16_t ThunderVisualStorage::AdvanceTimer(
    ThunderObjectState& state, double frame_time) {
    if ((state.words[15u] & 0xffu) == 0u) return 0u;

    const double elapsed =
        static_cast<double>(std::bit_cast<float>(state.words[36u])) +
        frame_time;
    state.words[36u] = std::bit_cast<std::uint32_t>(
        static_cast<float>(elapsed));
    const double interval =
        static_cast<double>(std::bit_cast<float>(state.words[35u]));
    if (!(elapsed >= interval)) return 0x100u;
    state.words[36u] = 0u;
    return 0u;
}

}  // namespace pal4
