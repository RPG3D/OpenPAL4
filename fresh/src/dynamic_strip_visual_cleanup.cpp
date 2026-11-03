#include "pal4/dynamic_strip_object_config.h"

namespace pal4 {

namespace {

std::uint8_t ReadByte(const StripVisualBlockView& state,
                      std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

void WriteByte(StripVisualBlockView state,
               std::uint32_t offset, std::uint8_t value) {
    auto& word = state.words[offset / 4u];
    const auto shift = 8u * (offset % 4u);
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint32_t DynamicStripObjectConfiguration::CleanupVisual(
    StripVisualBlockView state,
    IDynamicStripCleanupRuntime& runtime) {
    if (ReadByte(state, 0u) == 0u) return 0u;

    WriteByte(state, 0u, 0u);
    WriteByte(state, 2u, 0u);
    state.words[15u] = 0u;
    const auto sentinel = state.words[28u];
    for (auto node = runtime.FirstNode(sentinel); node != sentinel;
         --state.words[29u]) {
        const auto next = runtime.NextNode(node);
        runtime.UnlinkNode(node);
        runtime.FreeNode(node);
        node = next;
    }

    std::uint32_t result = state.words[16u];
    if (result != 0u) {
        result = runtime.DestroyTexture(result);
        state.words[16u] = 0u;
    }
    if (ReadByte(state, 3u) != 0u) {
        WriteByte(state, 3u, 0u);
        state.words[26u] = 0u;
        state.words[14u] = 0u;
    } else {
        if (state.words[25u] != 0u) {
            (void)runtime.DestroyFrame(state.words[25u]);
            state.words[25u] = 0u;
        }
        if (state.words[26u] != 0u) {
            (void)runtime.DestroyFrame(state.words[26u]);
            state.words[26u] = 0u;
        }
        if (state.words[14u] != 0u) {
            result = runtime.DestroyFrame(state.words[14u]);
            state.words[14u] = 0u;
        }
    }
    return result;
}

std::uint32_t DynamicStripObjectConfiguration::CleanupVisual(
    DynamicStripObjectState& state, IDynamicStripCleanupRuntime& runtime) {
    return CleanupVisual(StripVisualBlockView{std::span{state.words}.first<34u>()}, runtime);
}

}  // namespace pal4
