#include "pal4/scene_evf_record.h"

#include <array>
#include <cstdint>
#include <limits>

namespace pal4 {

bool SceneEvfRecord::IntersectsPixel(
    const SceneEvfRecordState& state, const std::uint32_t camera,
    const std::uint32_t pixel, float* distance,
    ISceneEvfIntersectionRuntime& runtime) {
    SceneEvfVec3 start{};
    SceneEvfVec3 end{};
    runtime.CalculatePixelRay(camera, pixel, start, end);
    runtime.TransformCameraRay(camera, start, end);

    float closest = std::numeric_limits<float>::max();
    bool found = false;
    for (std::uint32_t i = 0u; i < state.words[42u]; ++i) {
        const auto indices = runtime.ReadTriangleIndices(state.words[41u], i);
        std::array<SceneEvfVec3, 3u> points{};
        for (std::uint32_t vertex = 0u; vertex < 3u; ++vertex)
            points[vertex] = runtime.ReadPropertyPosition(state.words[21u],
                                                           indices[vertex]);
        runtime.TransformTriangle(camera, points);
        SceneEvfVec3 hit{};
        if (!runtime.IntersectLine(start, end, points, hit)) continue;
        const SceneEvfVec3 delta{start[0u] - hit[0u],
                                 start[1u] - hit[1u],
                                 start[2u] - hit[2u]};
        const auto candidate = runtime.Length(delta);
        if (candidate < closest) {
            closest = candidate;
            found = true;
        }
    }
    if (distance != nullptr) *distance = closest;
    return found;
}

}  // namespace pal4
