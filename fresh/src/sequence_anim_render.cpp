#include "pal4/sequence_anim_render.h"

#include <bit>
#include <stdexcept>

namespace pal4 {

std::uint8_t SequenceAnimRenderer::Render(
    const SequenceAnimObjectState& state, const std::uint32_t context,
    const std::uint32_t caller_ebp, const std::uint32_t caller_edi,
    ISequenceAnimRenderRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u ||
        ((state.words[1u] >> 24u) & 0xffu) == 0u ||
        !(std::bit_cast<float>(state.words[13u]) > 0.0f))
        return 0u;

    const auto resource = state.words[30u];
    const auto instance = state.words[3u];
    if (instance != 0u && runtime.ReadByte(instance + 83u) != 0u) {
        const auto object = runtime.ReadWord(resource + 20u);
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
    }
    return runtime.RenderResource(resource, caller_ebp, caller_edi);
}

}  // namespace pal4
