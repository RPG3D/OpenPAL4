#include "pal4/thunder_visual_storage.h"

#include <cstdint>

namespace pal4 {
namespace {

void WriteByte(ThunderObjectState& state, std::uint32_t offset,
               std::uint8_t value) {
    offset += 60u;
    auto& word = state.words[offset / 4u];
    const auto shift = 8u * (offset % 4u);
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

void ThunderVisualStorage::ResetProperties(
    ThunderObjectState& state, IThunderVisualResetRuntime& runtime) {
    if ((state.words[15u] & 0xffu) == 0u) return;

    WriteByte(state, 1u, 1u);
    WriteByte(state, 3u, 1u);
    state.words[18u] = 0x3f800000u;
    state.words[19u] = 0x3f800000u;
    state.words[20u] = 0x3f800000u;
    state.words[21u] = 0u;
    WriteByte(state, 0u, 0u);
    WriteByte(state, 2u, 0u);
    state.words[22u] = 0u;
    WriteByte(state, 4u, 0u);
    WriteByte(state, 5u, 0u);
    state.words[35u] = 0u;
    state.words[23u] = 0u;
    state.words[36u] = 0u;
    state.words[24u] = 0u;
    state.words[40u] = 1045220557u;
    state.words[25u] = 0u;
    state.words[43u] = 0x3f800000u;
    state.words[44u] = 1053609165u;
    state.words[26u] = 0u;
    state.words[37u] = 0u;
    state.words[38u] = 0u;
    state.words[27u] = 0xffffffffu;
    state.words[41u] = 0x3f000000u;
    state.words[42u] = 0x3f000000u;
    state.words[39u] = 0u;

    if (state.words[29u] != 0u) {
        runtime.DestroyTexture(state.words[29u]);
        state.words[29u] = 0u;
    }
    if (state.words[34u] != 0u) {
        runtime.DestroyFrame(state.words[34u]);
        state.words[34u] = 0u;
    }
    if (state.words[28u] != 0u) {
        runtime.DestroyFrame(state.words[28u]);
        state.words[28u] = 0u;
    }
}

}  // namespace pal4
