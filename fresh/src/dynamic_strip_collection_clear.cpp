#include "pal4/dynamic_strip_object_config.h"

namespace pal4 {

void DynamicStripObjectConfiguration::ClearNonHookSegments(
    StripVisualBlockView state, IDynamicStripCleanupRuntime& runtime) {
    if ((state.words[0u] >> 24u) != 0u) return;
    state.words[0u] |= 0x00010000u;

    const auto sentinel = state.words[28u];
    for (auto node = runtime.FirstNode(sentinel); node != sentinel;) {
        const auto next = runtime.NextNode(node);
        runtime.UnlinkNode(node);
        runtime.FreeNode(node);
        --state.words[29u];
        node = next;
    }
}
void DynamicStripObjectConfiguration::ClearNonHookSegments(
    DynamicStripObjectState& state, IDynamicStripCleanupRuntime& runtime) {
    ClearNonHookSegments(StripVisualBlockView{std::span{state.words}.first<34u>()},runtime);
}

}  // namespace pal4
