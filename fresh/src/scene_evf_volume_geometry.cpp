#include "pal4/scene_evf_record.h"

#include <array>
#include <cstdint>
#include <vector>

namespace pal4 {

bool SceneEvfRecord::GenerateVolumeGeometry(
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
    // Native cap setup writes more triangles than it allocates below eight records.
    if (count < 8u) {
        runtime.WarnInsufficientVolumeVertices();
        runtime.ClearPropertyRange(begin, end);
        state.words[22u] = begin;
        return false;
    }

    const auto half = count / 2u;
    const auto index_count = 3u * count + 4u * (3u * half - 6u);
    const auto triangle_count = 2u * (2u * half - 4u) + count;
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

    for (std::uint32_t i = 0u; i + 1u < half; ++i) {
        emit(i, half + i, i + 1u);
        emit(half + i, half + i + 1u, i + 1u);
    }
    emit(half - 2u, count - 1u, 0u);
    emit(count - 1u, half, 0u);

    const auto emit_cap = [&](const std::uint32_t base,
                              const std::uint32_t last) {
        std::vector<std::array<std::uint32_t, 3u>> front;
        front.reserve(half - 2u);
        front.push_back({base, base + 1u, last});
        front.push_back({base + 1u, base + 2u, last});
        for (std::uint32_t left = 2u; front.size() < half - 2u; ++left) {
            front.push_back({last + 2u - left, base + left,
                             last + 1u - left});
            if (front.size() < half - 2u)
                front.push_back({base + left, base + left + 1u,
                                 last + 1u - left});
        }
        for (const auto& triangle : front)
            emit(triangle[0u], triangle[1u], triangle[2u]);
        for (const auto& triangle : front)
            emit(triangle[0u], triangle[2u], triangle[1u]);
    };
    emit_cap(0u, half - 1u);
    emit_cap(half, count - 1u);

    runtime.WriteIndices(state.words[29u], indices);
    runtime.WriteTriangles(state.words[41u], triangles);
    return triangles.size() == triangle_count;
}

}  // namespace pal4
