#include "pal4/pal_rectangle_drawer.h"

#include <bit>

namespace pal4 {
namespace {

[[nodiscard]] std::uint32_t PackColor(const PalRectangleColor color) {
    return (static_cast<std::uint32_t>(color.alpha) << 24u) |
           (static_cast<std::uint32_t>(color.red) << 16u) |
           (static_cast<std::uint32_t>(color.green) << 8u) |
           static_cast<std::uint32_t>(color.blue);
}

[[nodiscard]] std::array<CeguiRenderVertex, 4> MakeVertices(
    const std::uint32_t context, const std::int32_t x,
    const std::int32_t y, const std::int32_t width,
    const std::int32_t height, const PalRectangleColor color,
    IPalRectangleRenderRuntime& runtime) {
    const auto right = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(x) + static_cast<std::uint32_t>(width));
    const auto bottom = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(height));
    const float reciprocal_depth = 1.0f / runtime.CameraDepth(context);
    const auto packed_color = PackColor(color);
    std::array<CeguiRenderVertex, 4> vertices{};
    vertices[0] = {static_cast<float>(x), static_cast<float>(bottom),
                   runtime.ActiveDepth(), reciprocal_depth,
                   packed_color, 0.0f, 0.0f};
    vertices[1] = {static_cast<float>(right), static_cast<float>(bottom),
                   runtime.ActiveDepth(), reciprocal_depth,
                   packed_color, 0.0f, 0.0f};
    vertices[2] = {static_cast<float>(x), static_cast<float>(y),
                   runtime.ActiveDepth(), reciprocal_depth,
                   packed_color, 0.0f, 0.0f};
    vertices[3] = {static_cast<float>(right), static_cast<float>(y),
                   runtime.ActiveDepth(), reciprocal_depth,
                   packed_color, 0.0f, 0.0f};
    return vertices;
}

void SetCommonStates(IPalRectangleRenderRuntime& runtime) {
    runtime.SetRenderState(6u, 0u);
    runtime.SetRenderState(8u, 0u);
    runtime.SetRenderState(7u, 2u);
    runtime.SetRenderState(9u, 2u);
    runtime.SetRenderState(2u, 3u);
}

[[nodiscard]] std::uint32_t DrawTexturedImpl(
    const std::uint32_t context, const std::int32_t x,
    const std::int32_t y, const std::int32_t width,
    const std::int32_t height, const std::uint32_t texture,
    const PalRectangleColor color, const bool alpha_enabled,
    IPalRectangleRenderRuntime& runtime) {
    if (context == 0u || texture == 0u) {
        return 0u;
    }
    auto vertices = MakeVertices(context, x, y, width, height,
                                 color, runtime);
    vertices[0].u = 0.0f;
    vertices[0].v = 1.0f;
    vertices[1].u = 1.0f;
    vertices[1].v = 1.0f;
    vertices[2].u = 0.0f;
    vertices[2].v = 0.0f;
    vertices[3].u = 1.0f;
    vertices[3].v = 0.0f;
    if (alpha_enabled) {
        runtime.SetRenderState(12u, 1u);
        runtime.SetRenderState(10u, 5u);
        runtime.SetRenderState(11u, 6u);
    }
    SetCommonStates(runtime);
    runtime.SetRenderState(1u, runtime.TextureRaster(texture));
    runtime.DrawPrimitive(4u, vertices, 4u);
    return 1u;
}

}  // namespace

float PalRectangleRenderRuntimeStub::CameraDepth(
    std::uint32_t /*context*/) { return 1.0f; }
float PalRectangleRenderRuntimeStub::ActiveDepth() { return 0.0f; }
std::uint32_t PalRectangleRenderRuntimeStub::TextureRaster(
    std::uint32_t /*texture*/) { return 0u; }
void PalRectangleRenderRuntimeStub::SetRenderState(
    std::uint32_t /*state*/, std::uint32_t /*value*/) {}
void PalRectangleRenderRuntimeStub::DrawPrimitive(
    std::uint32_t /*primitive*/,
    const std::array<CeguiRenderVertex, 4>& /*vertices*/,
    std::uint32_t /*count*/) {}

std::uint32_t PalRectangleDrawer::DrawFilled(
    const std::uint32_t context, const std::int32_t x,
    const std::int32_t y, const std::int32_t width,
    const std::int32_t height, const PalRectangleColor color,
    const std::array<std::uint32_t, 8>& incoming_uv_bits,
    IPalRectangleRenderRuntime& runtime) {
    if (context == 0u) {
        return 0u;
    }
    auto vertices = MakeVertices(context, x, y, width, height,
                                 color, runtime);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        vertices[index].u = std::bit_cast<float>(incoming_uv_bits[2u * index]);
        vertices[index].v = std::bit_cast<float>(incoming_uv_bits[2u * index + 1u]);
    }
    runtime.SetRenderState(12u, 1u);
    SetCommonStates(runtime);
    runtime.SetRenderState(1u, 0u);
    runtime.DrawPrimitive(4u, vertices, 4u);
    return 1u;
}

std::uint32_t PalRectangleDrawer::DrawTextured(
    const std::uint32_t context, const std::int32_t x,
    const std::int32_t y, const std::int32_t width,
    const std::int32_t height, const std::uint32_t texture,
    const PalRectangleColor color, IPalRectangleRenderRuntime& runtime) {
    return DrawTexturedImpl(context, x, y, width, height, texture,
                            color, false, runtime);
}

std::uint32_t PalRectangleDrawer::DrawTexturedWithAlpha(
    const std::uint32_t context, const std::int32_t x,
    const std::int32_t y, const std::int32_t width,
    const std::int32_t height, const std::uint32_t texture,
    const PalRectangleColor color, IPalRectangleRenderRuntime& runtime) {
    return DrawTexturedImpl(context, x, y, width, height, texture,
                            color, true, runtime);
}

}  // namespace pal4
