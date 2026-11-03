#include "pal4/sequence_anim_playback_facet.h"

namespace pal4 {
namespace {

std::uint8_t Byte(const SequenceAnimObjectState& state,
                  const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> ((offset % 4u) * 8u));
}

std::uint32_t Clump(const SequenceAnimObjectState& state,
                    ISequenceAnimPlaybackFacetRuntime& runtime) {
    const auto geometry = runtime.ReadWord(state.words[30u] + 48u);
    return runtime.ReadWord(geometry + 8u);
}

}  // namespace

std::uint8_t SequenceAnimPlaybackFacet::Stop(
    SequenceAnimObjectState& state,
    ISequenceAnimPlaybackFacetRuntime& runtime) {
    if (Byte(state, 4u) == 0u)
        return 0u;
    const auto running = Byte(state, 6u);
    if (running == 0u)
        return 0u;

    state.words[1u] &= 0x0000ffffu;
    const auto first_clump = Clump(state, runtime);
    const auto world = runtime.WorldForClump(first_clump);
    if (world == 0u)
        return 0u;
    return runtime.RemoveClump(world, Clump(state, runtime));
}

}  // namespace pal4
