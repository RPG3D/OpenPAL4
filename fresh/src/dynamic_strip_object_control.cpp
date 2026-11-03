#include "pal4/dynamic_strip_object_control.h"

namespace pal4 {
namespace {

void WriteByte(DynamicStripObjectState& state,
               const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) |
           (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint32_t DynamicStripObjectControl::StopAndDestroy(
    DynamicStripObjectState& state,
    IDynamicStripObjectControlRuntime& runtime) {
    if ((state.words[1u] & 0xffu) != 0u) {
        runtime.CleanupResources(state);
        const auto system = state.words[4u];
        WriteByte(state, 4u, 0u);
        WriteByte(state, 6u, 0u);
        WriteByte(state, 7u, 0u);
        runtime.ProcessEvent(system, state);
    }
    return runtime.DeleteSelf(state, 1u);
}

}  // namespace pal4
