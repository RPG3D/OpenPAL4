#include "pal4/strip_atomic_object.h"

#include <cmath>

namespace pal4 {

int StripAtomicObject::AppendCylinderPath(
    std::vector<StripPoint>& points, int border_count, float height,
    float outer_radius, float inner_radius, int loops) {
    if (loops <= 0) return loops;
    if (border_count <= 0) return 0;

    const float angle_step = static_cast<float>(
        6.2831855 / static_cast<double>(border_count));
    const float vertical_step = static_cast<float>(
        static_cast<double>(height) / static_cast<double>(loops) /
        static_cast<double>(border_count));
    const float radius_slope = (outer_radius - inner_radius) / height;
    float y = 0.0f;
    for (int loop = 0; loop < loops; ++loop) {
        for (int border = 0; border < border_count; ++border) {
            const float angle = static_cast<float>(
                static_cast<double>(border) * angle_step);
            y += vertical_step;
            const float radius = radius_slope * y + inner_radius;
            points.push_back({std::cos(angle) * radius, y,
                              std::sin(angle) * radius});
        }
    }
    return 0;
}

std::uint32_t StripAtomicObject::GetFrame(
    const StripAtomicState& state, IStripAtomicObjectRuntime& runtime) {
    return state.atomic == 0u ? 0u : runtime.FrameOf(state.atomic);
}

void StripAtomicObject::Create(
    StripAtomicState& state, std::uint32_t material,
    const std::vector<StripPoint>& points, float height,
    bool alternate_uv, bool blended, IStripAtomicCreateRuntime& runtime) {
    if (points.size() < 2u || !(height > 0.0f)) return;
    if (material == 0u) material = runtime.CreateMaterial();

    const auto count = static_cast<std::uint32_t>(points.size());
    const auto geometry = runtime.CreateGeometry(
        2u * count, 2u * count - 2u, 52u);
    if (geometry == 0u) return;

    const float half_height = height * 0.5f;
    for (std::uint32_t i = 0u; i < count; ++i) {
        const float fraction = static_cast<float>(i) /
                               static_cast<float>(count - 1u);
        const StripPoint top{points[i].x, points[i].y + half_height,
                             points[i].z};
        const StripPoint bottom{points[i].x, points[i].y - half_height,
                                points[i].z};
        const StripUv top_uv = alternate_uv
            ? StripUv{0.0f, fraction} : StripUv{fraction, 0.0f};
        const StripUv bottom_uv = alternate_uv
            ? StripUv{1.0f, fraction} : StripUv{fraction, 1.0f};
        runtime.WriteVertex(geometry, 2u * i, top, top_uv);
        runtime.WriteVertex(geometry, 2u * i + 1u, bottom, bottom_uv);
    }
    for (std::uint32_t i = 0u; i + 1u < count; ++i) {
        const auto first = 2u * i;
        runtime.SetTriangle(geometry, first, first + 2u,
                            first + 3u, first + 1u, material);
        runtime.SetTriangle(geometry, first + 1u, first,
                            first + 2u, first + 1u, material);
    }

    state.atomic = runtime.CreateAtomic();
    const auto frame = runtime.CreateFrame();
    runtime.SetFrameIdentity(frame);
    runtime.SetAtomicFrame(state.atomic, frame);
    runtime.SetAtomicGeometry(state.atomic, geometry);
    if (material != 0u) runtime.CalculateVertexNormals(geometry);
    const auto sphere = runtime.CalculateBoundingSphere(geometry);
    runtime.SetBoundingSphere(geometry, sphere);
    runtime.UnlockGeometry(geometry);
    runtime.DestroyGeometry(geometry);
    if (runtime.CachedOriginalCallback() == 0u)
        runtime.CacheOriginalCallback(runtime.AtomicCallback(state.atomic));
    runtime.InstallCallback(state.atomic, blended);
    if (runtime.AtomicCallback(state.atomic) == 0u)
        runtime.InstallDefaultCallback(state.atomic);
}

std::uint32_t StripAtomicObject::Destroy(
    StripAtomicState& state, IStripAtomicObjectRuntime& runtime) {
    if (state.atomic == 0u) return 0u;

    const auto geometry = runtime.GeometryOf(state.atomic);
    if (geometry != 0u) {
        const auto material = runtime.FirstMaterialOf(geometry);
        if (material != 0u) runtime.DestroyMaterial(material);
    }
    runtime.DestroyFrame(runtime.FrameOf(state.atomic));
    runtime.SetAtomicFrame(state.atomic, 0u);
    const auto result = runtime.DestroyAtomic(state.atomic);
    state.atomic = 0u;
    return result;
}

}  // namespace pal4
