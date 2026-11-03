#include "pal4/game_config_state.h"

namespace pal4 {

std::uint32_t GameConfigState::FreeAndNull(std::uint32_t& slot,
                                           IGameConfigMemory& memory) {
    if (slot == 0) return 0;
    const auto result = memory.Free(slot);
    slot = 0;
    return result;
}

GameConfigState& GameConfigState::Initialize() {
    words.fill(0);
    words[6] = 0x3f800000u;
    words[7] = 0x3f800000u;
    words[13] = 3;
    return *this;
}

std::uint32_t GameConfigState::Cleanup(IGameConfigMemory& memory) {
    for (std::size_t i = 0; i < 6; ++i) words[i] = 0;
    words[13] = 3;
    for (std::size_t i = 9; i <= 12; ++i) {
        (void)FreeAndNull(words[i], memory);
    }
    const auto result = words[14];
    words[14] = 0;
    words[15] = 0;
    return result;
}

}  // namespace pal4
