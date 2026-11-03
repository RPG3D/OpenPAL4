#include "pal4/dynamic_strip_resource_block.h"

namespace pal4 {

void DynamicStripResourceBlock::Cleanup(
    DynamicStripObjectState& state,
    IDynamicStripResourceBlockRuntime& runtime) {
    runtime.CleanupVisual(state);
    ReleaseStorage(StripVisualBlockView{std::span{state.words}.subspan<13u,34u>()}, runtime);
}

void DynamicStripResourceBlock::ReleaseStorage(
    StripVisualBlockView state, IStripResourceStorageRuntime& runtime) {
    runtime.Free(state.words[31u]);
    state.words[31u] = 0u;
    state.words[32u] = 0u;
    state.words[33u] = 0u;

    const auto sentinel = state.words[28u];
    if (!sentinel) return;
    auto node = runtime.ReadWord(sentinel);
    while (node != sentinel) {
        const auto next = runtime.ReadWord(node);
        const auto previous = runtime.ReadWord(node + 4u);
        runtime.WriteWord(previous, next);
        runtime.WriteWord(next + 4u, previous);
        runtime.Free(node);
        --state.words[29u];
        node = next;
    }
    runtime.Free(sentinel);
    state.words[28u] = 0u;
    state.words[29u] = 0u;
}

}  // namespace pal4
