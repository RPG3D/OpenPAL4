#include "pal4/thunder_flag.h"

namespace pal4 {

std::uint8_t ThunderFlag::Get(const ThunderObjectState& state) {
    return static_cast<std::uint8_t>(state.words[15u] >> 16u);
}

std::uint8_t ThunderFlag::Set(ThunderObjectState& state,
                              const std::uint8_t value) {
    state.words[15u] = (state.words[15u] & 0xff00ffffu) |
                       (static_cast<std::uint32_t>(value) << 16u);
    return value;
}

}  // namespace pal4
