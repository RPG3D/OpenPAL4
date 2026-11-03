#include "pal4/strip_renderer.h"

#include <stdexcept>

namespace pal4 {
namespace {

std::uint8_t Byte(const StripObjectState& state, std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

}  // namespace

std::uint8_t StripRenderer::Render(
    const StripObjectState& state, const std::uint32_t context,
    const std::uint32_t caller_ebp, const std::uint32_t caller_edi,
    IStripRenderRuntime& runtime) {
    const auto active = Byte(state, 4u);
    if (active == 0u)
        return active;
    const auto visible = Byte(state, 7u);
    if (visible == 0u)
        return visible;

    const auto straight = Byte(state, 140u) != 0u;
    const auto instance = state.words[3u];
    if (instance != 0u && runtime.ReadByte(instance + 83u) != 0u) {
        const auto resource = straight ? state.words[38u] : state.words[34u];
        const auto object = runtime.ReadWord(resource + (straight ? 16u : 4u));
        const auto translation = runtime.GetObjectTransform(object) + 48u;
        const auto manager = runtime.EffectManager();
        if (manager == 0u) {
            throw std::runtime_error(
                "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
                "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
                "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1");
        }
        const auto valid = runtime.ValidateTransform(
            manager, context, translation);
        if (valid == 0u)
            return valid;
    }
    if (straight)
        return runtime.RenderStraight(state.words[38u], caller_ebp, caller_edi);
    return runtime.RenderClump(state.words[34u]);
}

}  // namespace pal4
