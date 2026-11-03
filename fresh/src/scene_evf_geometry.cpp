#include "pal4/scene_evf_record.h"

#include <cstdint>

namespace pal4 {

bool SceneEvfRecord::GenerateStripGeometry(
    SceneEvfRecordState& state, const bool double_sided,
    const bool closed, ISceneEvfGeometryRuntime& runtime) {
    for (const auto offset : {116u, 124u, 164u}) {
        auto& pointer = state.words[offset / 4u];
        if (pointer != 0u) {
            runtime.Free(pointer);
            pointer = 0u;
        }
    }
    const auto begin = state.words[21u];
    const auto end = state.words[22u];
    const auto count = begin == 0u ? 0u : (end - begin) / 80u;
    state.words[32u] = count;
    if (count <= 1u) {
        runtime.WarnInsufficientVertices();
        runtime.ClearPropertyRange(begin, end);
        state.words[22u] = begin;
        return false;
    }

    auto index_count = closed ? 3u * count : 3u * count - 6u;
    if (double_sided) index_count *= 2u;
    const auto triangle_count = index_count / 3u;
    state.words[30u] = index_count;
    state.words[42u] = triangle_count;
    state.words[29u] = runtime.Allocate(2u * index_count);
    state.words[31u] = runtime.Allocate(36u * count);
    state.words[41u] = runtime.Allocate(28u * triangle_count);

    const auto properties = runtime.ReadProperties(begin, count);
    const auto color = runtime.ReadColor(state.words[33u]);
    const auto packed_color =
        (static_cast<std::uint32_t>(color[3u]) << 24u) |
        (static_cast<std::uint32_t>(color[0u]) << 16u) |
        (static_cast<std::uint32_t>(color[1u]) << 8u) |
        static_cast<std::uint32_t>(color[2u]);
    std::vector<SceneEvfVertex> vertices(count);
    for (std::uint32_t i = 0u; i < count; ++i) {
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            vertices[i].words[axis] = properties[i].words[axis];
        vertices[i].words[6u] = packed_color;
    }
    runtime.WriteVertices(state.words[31u], vertices);

    std::vector<std::uint16_t> indices;
    std::vector<SceneEvfTriangle> triangles;
    indices.reserve(index_count);
    triangles.reserve(triangle_count);
    const auto emit = [&](const std::uint32_t a, const std::uint32_t b,
                          const std::uint32_t c) {
        const auto first = static_cast<std::uint16_t>(a);
        const auto second = static_cast<std::uint16_t>(b);
        const auto third = static_cast<std::uint16_t>(c);
        indices.insert(indices.end(), {first, second, third});
        SceneEvfTriangle triangle;
        triangle.indices = {first, second, third};
        triangle.distance = runtime.PlaneFromPoints(
            properties[first], properties[second], properties[third],
            triangle.normal);
        triangles.push_back(triangle);
    };

    const auto half = count >> 1u;
    for (std::uint32_t i = 0u; i + 1u < half; ++i) {
        emit(i, half + i, i + 1u);
        emit(half + i, half + i + 1u, i + 1u);
    }
    if (closed) {
        emit(half - 1u, count - 1u, 0u);
        emit(count - 1u, half, 0u);
    }
    if (double_sided) {
        for (std::uint32_t i = half - 1u; i > 0u; --i) {
            emit(i, half + i, i - 1u);
            emit(half + i, half + i - 1u, i - 1u);
        }
        if (closed) {
            emit(0u, half, half - 1u);
            emit(half, count - 1u, half - 1u);
        }
    }
    runtime.WriteIndices(state.words[29u], indices);
    runtime.WriteTriangles(state.words[41u], triangles);
    return true;
}

}  // namespace pal4
