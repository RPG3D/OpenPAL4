#include "pal4/dynamic_strip_object_render.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace pal4 {
namespace {

std::uint8_t Byte(StripVisualBlockConstView state, std::size_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     (8u * (offset % 4u)));
}

float Float(StripVisualBlockConstView state, std::size_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

float ClampOne(double value) {
    return value < 1.0 ? static_cast<float>(value) : 1.0f;
}

std::uint8_t Alpha(float authored, float remaining, float life) {
    const double scaled = static_cast<double>(authored) *
                          static_cast<double>(remaining) /
                          static_cast<double>(life);
    // CRT_Ftol obeys the x87 rounding mode; the default is nearest-even.
    if (!std::isfinite(scaled) ||
        scaled < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
        scaled > static_cast<double>(std::numeric_limits<std::int32_t>::max()))
        return 0u;
    return static_cast<std::uint8_t>(
        static_cast<std::int32_t>(std::nearbyint(scaled)));
}

void AppendVertex(std::vector<DynamicStripIm3DVertex>& vertices,
                  const std::array<float, 3u>& point, std::uint32_t color,
                  float u, float v) {
    vertices.push_back({point[0], point[1], point[2],
                        0.0f, 0.0f, 0.0f, color, u, v});
}

}  // namespace

void DynamicStripObjectRenderer::RenderVisual(
    StripVisualBlockConstView state,
    IDynamicStripVisualRenderRuntime& runtime) {
    if (Byte(state, 0u) == 0u ||
        static_cast<std::int32_t>(state.words[29u]) < 2)
        return;

    auto segments = runtime.Segments(state.words[28u]);
    if (Byte(state, 4u) != 0u) {
        if (segments.empty()) return;
        segments = {segments.front(), segments.back()};
    }
    if (segments.size() < 2u) return;

    std::vector<DynamicStripIm3DVertex> vertices;
    vertices.reserve((segments.size() - 1u) * 4u);
    const auto rgb = (static_cast<std::uint32_t>(Byte(state, 52u)) << 16u) |
                     (static_cast<std::uint32_t>(Byte(state, 53u)) << 8u) |
                     Byte(state, 54u);
    const auto authored_alpha = static_cast<float>(Byte(state, 55u));
    const auto life = Float(state, 60u);
    const auto base_u = Float(state, 68u);
    const auto base_v = Float(state, 72u);
    const auto upper_v = ClampOne(static_cast<double>(base_v) + 1.0);
    const double step = 1.0 / static_cast<double>(segments.size() - 1u);
    for (std::size_t i = 0; i + 1u < segments.size(); ++i) {
        const auto& first = segments[i];
        const auto& second = segments[i + 1u];
        const auto first_color =
            (static_cast<std::uint32_t>(Alpha(authored_alpha,
                                              first.remaining, life)) << 24u) | rgb;
        const auto second_color =
            (static_cast<std::uint32_t>(Alpha(authored_alpha,
                                              second.remaining, life)) << 24u) | rgb;
        const auto first_u = ClampOne(base_u + static_cast<double>(i) * step);
        const auto second_u = ClampOne(base_u + static_cast<double>(i + 1u) * step);
        AppendVertex(vertices, first.first, first_color, first_u, base_v);
        AppendVertex(vertices, first.second, first_color, first_u, upper_v);
        AppendVertex(vertices, second.first, second_color, second_u, base_v);
        AppendVertex(vertices, second.second, second_color, second_u, upper_v);
    }

    std::vector<std::uint16_t> indices;
    indices.reserve(vertices.size() / 4u * 6u);
    for (std::size_t i = 0; i < vertices.size(); i += 4u) {
        for (const auto offset : {0u, 2u, 1u, 1u, 2u, 3u})
            indices.push_back(static_cast<std::uint16_t>(i + offset));
    }

    runtime.SetDeviceState(1u, runtime.TextureRaster(state.words[16u]));
    const auto old27 = runtime.GetRenderState(27u);
    const auto old19 = runtime.GetRenderState(19u);
    const auto old20 = runtime.GetRenderState(20u);
    runtime.SetRenderState(27u, 1u);
    runtime.SetRenderState(19u, 5u);
    runtime.SetRenderState(20u, Byte(state, 1u) != 0u ? 2u : 6u);
    const auto old7 = runtime.GetRenderState(7u);
    const auto old14 = runtime.GetRenderState(14u);
    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(14u, 0u);
    runtime.SetDeviceState(9u, 2u);
    const auto old_device20 = runtime.GetDeviceState(20u);
    runtime.SetDeviceState(20u, 1u);
    runtime.Transform(vertices, 29u);
    runtime.RenderIndexed(3u, indices);
    runtime.End();
    runtime.SetDeviceState(1u, 0u);
    runtime.SetDeviceState(20u, old_device20);
    runtime.SetRenderState(7u, old7);
    runtime.SetRenderState(14u, old14);
    runtime.SetRenderState(27u, old27);
    runtime.SetRenderState(19u, old19);
    runtime.SetRenderState(20u, old20);
}

void DynamicStripObjectRenderer::RenderVisual(
    const DynamicStripObjectState& state,
    IDynamicStripVisualRenderRuntime& runtime) {
    RenderVisual(StripVisualBlockConstView{std::span{state.words}.first<34u>()}, runtime);
}

}  // namespace pal4
