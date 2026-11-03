#include "pal4/scene_evf_record.h"

#include <cstdint>

namespace pal4 {

bool SceneEvfRecord::GeneratePolygonGeometry(
    SceneEvfRecordState& state, ISceneEvfGeometryRuntime& runtime) {
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
    if (count <= 3u) {
        runtime.WarnInsufficientPolygonVertices();
        runtime.ClearPropertyRange(begin, end);
        state.words[22u] = begin;
        return false;
    }

    const auto triangle_count = 2u * count - 4u;
    const auto index_count = 3u * triangle_count;
    state.words[30u] = index_count;
    state.words[42u] = triangle_count;
    state.words[31u] = runtime.Allocate(36u * count);
    state.words[29u] = runtime.Allocate(2u * index_count);
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

    std::vector<std::array<std::uint32_t, 3u>> front;
    front.reserve(count - 2u);
    front.push_back({0u, 1u, count - 1u});
    front.push_back({1u, 2u, count - 1u});
    for (std::uint32_t left = 2u; front.size() < count - 2u; ++left) {
        front.push_back({count + 1u - left, left, count - left});
        if (front.size() < count - 2u)
            front.push_back({left, left + 1u, count - left});
    }

    std::vector<std::uint16_t> indices;
    std::vector<SceneEvfTriangle> triangles;
    indices.reserve(index_count);
    triangles.reserve(triangle_count);
    const auto emit = [&](const std::array<std::uint32_t, 3u>& input) {
        const auto first = static_cast<std::uint16_t>(input[0u]);
        const auto second = static_cast<std::uint16_t>(input[1u]);
        const auto third = static_cast<std::uint16_t>(input[2u]);
        indices.insert(indices.end(), {first, second, third});
        SceneEvfTriangle triangle;
        triangle.indices = {first, second, third};
        triangle.distance = runtime.PlaneFromPoints(
            properties[first], properties[second], properties[third],
            triangle.normal);
        triangles.push_back(triangle);
    };
    for (const auto& triangle : front) emit(triangle);
    for (const auto& triangle : front)
        emit({triangle[0u], triangle[2u], triangle[1u]});
    runtime.WriteIndices(state.words[29u], indices);
    runtime.WriteTriangles(state.words[41u], triangles);
    return true;
}

}  // namespace pal4
