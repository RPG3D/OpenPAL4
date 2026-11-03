#include "pal4/thunder_renderer.h"

#include <stdexcept>

namespace pal4 {
namespace {

constexpr char kMissingManager[] =
    "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
    "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
    "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1";

}  // namespace

void ThunderRenderer::Render(ThunderObjectState& state,
                             const std::uint32_t context,
                             IThunderRenderRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u ||
        ((state.words[1u] >> 24u) & 0xffu) == 0u)
        return;
    const auto owner = state.words[3u];
    if (owner != 0u && runtime.OwnerVisible(owner, 83u)) {
        const auto transform = runtime.ObjectTransform(state.words[28u]) + 48u;
        if (!runtime.EffectManagerAvailable())
            throw std::runtime_error(kMissingManager);
        if (!runtime.ValidateTransform(context, transform))
            return;
    }
    runtime.RenderVisual(state);
}

}  // namespace pal4
