#include "pal4/effect_object_render.h"

#include <stdexcept>

namespace pal4 {

void EffectObjectRenderer::Render(
    const EffectObjectState& state, const std::uint32_t context,
    IEffectObjectRenderRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u ||
        ((state.words[1u] >> 24u) & 0xffu) == 0u)
        return;

    const auto instance = state.words[23u];
    if (instance != 0u && runtime.ReadByte(instance + 83u) != 0u) {
        const auto object = runtime.ReadWord(instance + 172u);
        const auto transform = runtime.GetObjectTransform(object) + 48u;
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
    runtime.NoOpDraw(context);
}

}  // namespace pal4
