#include "pal4/cylinder_object_control.h"

#include <bit>

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

std::uint32_t CylinderObjectControl::StopAndDestroy(
    CylinderObjectState& state, ICylinderObjectControlRuntime& runtime) {
    if ((state.words[1u] & 0xffu) != 0u) {
        runtime.StopFacet(state, 48u);
        runtime.StopFacet(state, 20u);
        WriteByte(state, 4u, 0u);
        WriteByte(state, 6u, 0u);
        WriteByte(state, 7u, 0u);
        state.words[15u] = 0u;
        state.words[16u] = std::bit_cast<std::uint32_t>(1.0f);
        state.words[17u] = std::bit_cast<std::uint32_t>(1.0f);
        state.words[18u] = 0u;
        state.words[19u] = 0u;
        WriteByte(state, 84u, 0u);
        state.words[20u] = 0u;
        const auto model = state.words[22u];
        if (model != 0u) {
            runtime.DestroyModel(model);
            runtime.FreeModel(model);
            state.words[22u] = 0u;
        }
        const auto document = state.words[23u];
        if (document != 0u) {
            runtime.DestroyDocument(document);
            runtime.FreeDocument(document);
            state.words[23u] = 0u;
        }
        runtime.ProcessEvent(state.words[4u], state);
    }
    return runtime.DeleteSelf(state, 1u);
}

}  // namespace pal4
