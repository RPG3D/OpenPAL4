#include "pal4/strip_world_facet.h"

namespace pal4 {
namespace {

std::uint8_t Byte(const StripObjectState& state, std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

void WriteByte(StripObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint8_t StripWorldFacet::Remove(
    StripObjectState& state, IStripWorldFacetRuntime& runtime) {
    const auto active = Byte(state, 4u);
    if (active == 0u)
        return active;
    const auto started = Byte(state, 6u);
    if (started == 0u)
        return started;
    const auto clump = state.words[34u];
    WriteByte(state, 6u, 0u);
    WriteByte(state, 7u, 0u);
    const auto world = runtime.ResolveWorld(clump);
    if (world == 0u)
        return 0u;
    return runtime.RemoveClump(world, clump);
}

}  // namespace pal4
