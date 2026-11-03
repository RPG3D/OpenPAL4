#include "pal4/cylinder_object_visibility.h"

namespace pal4 {
namespace {

void WriteByte(CylinderObjectState& state,
               const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) |
           (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint8_t CylinderObjectVisibilityFacet::Deactivate(
    CylinderObjectState& state, ICylinderObjectVisibilityRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    WriteByte(state, 6u, 0u);
    WriteByte(state, 7u, 0u);
    const auto clump = runtime.ReadWord(state.words[22u] + 4u);
    const auto world = runtime.FindWorld(clump);
    if (world == 0u)
        return 0u;
    return runtime.RemoveClump(
        world, runtime.ReadWord(state.words[22u] + 4u));
}

}  // namespace pal4
