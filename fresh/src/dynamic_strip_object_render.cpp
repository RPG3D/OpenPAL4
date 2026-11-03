#include "pal4/dynamic_strip_object_render.h"

#include <stdexcept>

namespace pal4 {

void DynamicStripObjectRenderer::Render(
    const DynamicStripObjectState& state,
    const std::uint32_t context,
    IDynamicStripObjectRenderRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u ||
        ((state.words[1u] >> 24u) & 0xffu) == 0u)
        return;

    const auto owner = state.words[3u];
    if (owner != 0u && runtime.ReadByte(owner + 83u) != 0u) {
        const auto transform =
            runtime.GetObjectTransform(state.words[27u]) + 48u;
        const auto manager = runtime.EffectManager();
        if (manager == 0u) {
            throw std::runtime_error(
                "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
                "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
                "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1");
        }
        if (runtime.ValidateTransform(manager, context, transform) == 0u)
            return;
    }
    runtime.RenderVisual(state);
}

}  // namespace pal4
