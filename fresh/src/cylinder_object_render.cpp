#include "pal4/cylinder_object_render.h"

#include <stdexcept>

namespace pal4 {

std::uint8_t CylinderObjectRenderer::Render(
    const CylinderObjectState& state, const std::uint32_t ebp_value,
    const std::uint32_t context,
    ICylinderObjectRenderRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    const auto render_enabled =
        static_cast<std::uint8_t>(state.words[1u] >> 24u);
    if (render_enabled == 0u)
        return render_enabled;

    const auto model = state.words[22u];
    const auto owner = state.words[3u];
    if (owner == 0u || runtime.ReadByte(owner + 83u) == 0u)
        return runtime.RenderModel(model, ebp_value, context, context);

    const auto object = runtime.ReadWord(model + 8u);
    const auto transform = runtime.GetObjectTransform(object) + 48u;
    const auto manager = runtime.EffectManager();
    if (manager == 0u) {
        throw std::runtime_error(
            "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
            "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
            "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1");
    }
    if (runtime.ValidateTransform(manager, context, transform) == 0u)
        return 0u;
    return runtime.RenderModel(model, ebp_value, context, context);
}

}  // namespace pal4
