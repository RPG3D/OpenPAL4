#include "pal4/thunder_renderer.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace pal4 {
namespace {

std::uint8_t Byte(const ThunderObjectState& state, std::size_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     (8u * (offset % 4u)));
}

ThunderIm3DVertex Vertex(const std::array<float, 3u>& position,
                         std::uint32_t color, float u, float v) {
    return {position[0], position[1], position[2],
            0.0f, 0.0f, 0.0f, color, u, v};
}

}  // namespace

void ThunderRenderer::RenderVisual(
    ThunderObjectState& state, IThunderVisualRenderRuntime& runtime) {
    if (Byte(state, 60u) == 0u) return;

    constexpr double epsilon =
        static_cast<double>(std::bit_cast<float>(0x38d1b717u));
    const double timer =
        static_cast<double>(std::bit_cast<float>(state.words[36u]));
    if (timer + epsilon > 0.0 && timer - epsilon < 0.0) {
        runtime.UpdateTransformProperties(state);
        runtime.UpdateVisual(state);
        runtime.UpdateCollection(state);
    }

    const auto begin = state.words[47u];
    const auto end = state.words[48u];
    if (begin == 0u ||
        static_cast<std::int32_t>(end - begin) / 24 < 2)
        return;
    const auto segments = runtime.ReadSegments(begin, end);
    if (segments.size() < 2u) return;

    const std::uint32_t color =
        (static_cast<std::uint32_t>(Byte(state, 111u)) << 24u) |
        (static_cast<std::uint32_t>(Byte(state, 108u)) << 16u) |
        (static_cast<std::uint32_t>(Byte(state, 109u)) << 8u) |
        Byte(state, 110u);
    std::vector<std::array<ThunderIm3DVertex, 4u>> quads;
    quads.reserve(segments.size() - 1u);
    for (std::size_t i = 0u; i + 1u < segments.size(); ++i) {
        const auto& first = segments[i];
        const auto& second = segments[i + 1u];
        quads.push_back({Vertex(first.first, color, 0.0f, 0.0f),
                         Vertex(first.second, color, 0.0f, 1.0f),
                         Vertex(second.first, color, 1.0f, 0.0f),
                         Vertex(second.second, color, 1.0f, 1.0f)});
    }

    runtime.SetDeviceState(1u, runtime.TextureRaster(state.words[29u]));
    const auto old27 = runtime.GetRenderState(27u);
    const auto old19 = runtime.GetRenderState(19u);
    const auto old20 = runtime.GetRenderState(20u);
    runtime.SetRenderState(27u, 1u);
    runtime.SetRenderState(19u, 5u);
    runtime.SetRenderState(20u, Byte(state, 61u) != 0u ? 2u : 6u);
    const auto old7 = runtime.GetRenderState(7u);
    const auto old14 = runtime.GetRenderState(14u);
    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(14u, 0u);
    runtime.SetDeviceState(9u, 2u);
    const auto old_device20 = runtime.GetDeviceState(20u);
    runtime.SetDeviceState(20u, 1u);
    constexpr std::array<std::uint16_t, 4u> indices{0u, 1u, 2u, 3u};
    for (const auto& quad : quads) {
        runtime.TransformQuad(quad, 29u);
        runtime.RenderIndexed(4u, indices);
        runtime.End();
    }
    runtime.SetDeviceState(1u, 0u);
    runtime.SetDeviceState(20u, old_device20);
    runtime.SetRenderState(7u, old7);
    runtime.SetRenderState(14u, old14);
    runtime.SetRenderState(27u, old27);
    runtime.SetRenderState(19u, old19);
    runtime.SetRenderState(20u, old20);
    state.words[52u] = state.words[51u];
}

}  // namespace pal4
