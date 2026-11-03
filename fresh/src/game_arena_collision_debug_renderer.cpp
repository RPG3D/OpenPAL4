#include "pal4/game_arena_collision_debug_renderer.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kVertexStride = 36u;
constexpr std::uint32_t kVerticesPerTriangle = 6u;
constexpr std::uint32_t kLineColor = 0xdc1414f0u;

void WriteVertex(IGameArenaCollisionDebugRuntime& runtime,
                 std::uint32_t buffer, std::uint32_t vertex,
                 const std::uint32_t (&position)[3]) {
    const auto address = buffer + vertex * kVertexStride;
    runtime.WriteWord(address, position[0]);
    runtime.WriteWord(address + 4u, position[1]);
    runtime.WriteWord(address + 8u, position[2]);
    runtime.WriteWord(address + 24u, kLineColor);
}

}  // namespace

void GameArenaCollisionDebugRenderer::Render(
    std::uint32_t tree, IGameArenaCollisionDebugRuntime& runtime) {
    const auto first = runtime.ReadWord(tree + 4u);
    if (first == 0u) return;
    const auto end = runtime.ReadWord(tree + 8u);
    const auto triangleCount =
        static_cast<std::int32_t>(end - first) >> 2;
    const auto vertexCount =
        static_cast<std::uint32_t>(triangleCount) * kVerticesPerTriangle;
    if (static_cast<std::int32_t>(vertexCount) <= 0) return;

    const auto buffer = runtime.Allocate(vertexCount * kVertexStride, 0u);
    for (std::int32_t index = 0; index < triangleCount; ++index) {
        std::uint32_t position[3][3];
        for (std::uint32_t vertex = 0; vertex < 3u; ++vertex) {
            for (std::uint32_t component = 0; component < 3u; ++component) {
                const auto entries = runtime.ReadWord(tree + 4u);
                const auto triangle = runtime.ReadWord(
                    entries + static_cast<std::uint32_t>(index) * 4u);
                const auto source = runtime.GetVertexAddress(triangle, vertex);
                position[vertex][component] =
                    runtime.ReadWord(source + component * 4u);
            }
        }
        const auto base = static_cast<std::uint32_t>(index) *
                          kVerticesPerTriangle;
        WriteVertex(runtime, buffer, base, position[0]);
        WriteVertex(runtime, buffer, base + 1u, position[1]);
        WriteVertex(runtime, buffer, base + 2u, position[1]);
        const auto fourth = buffer + (base + 3u) * kVertexStride;
        runtime.WriteWord(fourth + 8u, position[2][2]);
        runtime.WriteWord(fourth, position[2][0]);
        runtime.WriteWord(fourth + 4u, position[2][1]);
        runtime.WriteWord(fourth + 24u, kLineColor);
        const auto fifth = buffer + (base + 4u) * kVertexStride;
        runtime.WriteWord(fifth + 8u, position[2][2]);
        runtime.WriteWord(fifth, position[2][0]);
        runtime.WriteWord(fifth + 4u, position[2][1]);
        runtime.WriteWord(fifth + 24u, kLineColor);
        WriteVertex(runtime, buffer, base + 5u, position[0]);
    }

    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(1u, 0u);
    if (runtime.Transform(buffer, vertexCount, 0u, 0u) != 0u) {
        runtime.RenderPrimitive(1u);
        runtime.EndTransform();
    }
    runtime.SetRenderState(6u, 1u);
    runtime.SetRenderState(7u, 2u);
    if (buffer != 0u) runtime.Free(buffer);
}

}  // namespace pal4
