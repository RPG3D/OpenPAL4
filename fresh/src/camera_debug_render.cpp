#include "pal4/camera_debug_render.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::array<std::uint16_t, 8> kBoxIndices{
    0, 1, 2, 3, 4, 5, 6, 7};
constexpr std::array<std::uint16_t, 6> kAxisIndices{0, 1, 2, 3, 4, 5};

std::uint32_t PackArgb(std::uint32_t rgba) {
    return (rgba & 0xff000000u) |
           ((rgba & 0x000000ffu) << 16u) |
           (rgba & 0x0000ff00u) |
           ((rgba & 0x00ff0000u) >> 16u);
}

float FloatAt(const CameraDataState& camera, std::size_t offset) {
    return std::bit_cast<float>(camera.At(offset));
}

}  // namespace

std::uint32_t CameraDebugRenderer::Render(CameraDataState& camera) {
    if (camera.At(0x1acu) != 0u) RenderFrustum(camera);
    if (camera.At(0x1b0u) != 0u) return RenderAxes(camera);
    return camera.At(0x1b0u);
}

void CameraDebugRenderer::RenderFrustum(CameraDataState& camera) {
    const auto device = runtime_.Device();
    const auto saved = runtime_.GetD3DRenderState(device, 28u);
    static_cast<void>(runtime_.SetD3DRenderState(device, 28u, 0u));
    runtime_.DrawBoundingBox(camera, 0x230u, 0xc7f93737u, 0xa8u);
    runtime_.SetRenderState(7u, 1u);
    runtime_.SetRenderState(1u, 0u);

    std::array<CameraDebugVertex, 8> vertices{};
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const auto position_offset = 0x1c0u + i * 12u;
        vertices[i].position = {
            FloatAt(camera, position_offset),
            FloatAt(camera, position_offset + 4u),
            FloatAt(camera, position_offset + 8u)};
        vertices[i].argb = PackArgb(camera.At(0x220u + i / 2u * 4u));
    }
    if (runtime_.Transform(vertices, 2u)) {
        runtime_.RenderIndexed(1u, kBoxIndices);
        runtime_.End();
    }
    runtime_.SetRenderState(6u, 1u);
    runtime_.SetRenderState(7u, 2u);
    static_cast<void>(runtime_.SetD3DRenderState(device, 28u, saved));
}

std::uint32_t CameraDebugRenderer::RenderAxes(CameraDataState& camera) {
    const auto device = runtime_.Device();
    const auto saved = runtime_.GetD3DRenderState(device, 28u);
    static_cast<void>(runtime_.SetD3DRenderState(device, 28u, 0u));
    runtime_.SetRenderState(7u, 1u);
    runtime_.SetRenderState(1u, 0u);

    const std::array<float, 3> origin{
        FloatAt(camera, 0x74u), FloatAt(camera, 0x78u),
        FloatAt(camera, 0x7cu)};
    std::array<CameraDebugVertex, 6> vertices{};
    constexpr std::array<std::uint32_t, 3> colors{
        0xffff4545u, 0xff45ff45u, 0xff4545ffu};
    for (std::size_t axis = 0; axis < 3u; ++axis) {
        vertices[axis * 2u] = {origin, colors[axis]};
        auto end = origin;
        end[axis] += 20.0f;
        vertices[axis * 2u + 1u] = {end, colors[axis]};
    }
    if (runtime_.Transform(vertices, 2u)) {
        runtime_.RenderIndexed(1u, kAxisIndices);
        runtime_.End();
    }
    runtime_.SetRenderState(6u, 1u);
    runtime_.SetRenderState(7u, 2u);
    return runtime_.SetD3DRenderState(device, 28u, saved);
}

}  // namespace pal4
