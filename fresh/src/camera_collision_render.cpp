#include "pal4/camera_collision_render.h"

namespace pal4 {
namespace {

std::uint32_t ReplicateByte(const std::uint8_t value) {
    const auto word = static_cast<std::uint32_t>(value);
    return word | (word << 8u) | (word << 16u) | (word << 24u);
}

bool AllBelow(const std::array<std::array<float, 3u>, 3u>& points,
              const std::size_t axis, const float limit) {
    return points[0][axis] < limit && points[1][axis] < limit &&
           points[2][axis] < limit;
}

bool AllAbove(const std::array<std::array<float, 3u>, 3u>& points,
              const std::size_t axis, const float limit) {
    return points[0][axis] > limit && points[1][axis] > limit &&
           points[2][axis] > limit;
}

}  // namespace

std::uint32_t CameraCollisionRender::RenderWorld(
    const std::uint32_t camera, const std::uint32_t texture,
    const std::uint32_t world, const std::uint32_t intersection,
    float intensity, const std::uint32_t unused,
    const std::uint32_t render_mode,
    ICameraCollisionRenderRuntime& runtime) {
    (void)unused;
    (void)runtime.SetDeviceState(8u, 0u);
    (void)runtime.SetDeviceState(1u, texture);
    (void)runtime.SetDeviceState(2u, 3u);
    (void)runtime.SetDeviceState(12u, 1u);
    (void)runtime.SetDeviceState(9u, 2u);
    (void)runtime.SetDeviceState(10u, 1u);
    if (intensity >= 0.0f) {
        (void)runtime.SetDeviceState(11u, 4u);
    } else {
        intensity = -intensity;
        (void)runtime.SetDeviceState(11u, 3u);
    }

    const auto frame = runtime.ReadWord(camera + 4u);
    CollisionRenderContext context{};
    context.camera_position = {
        runtime.ReadFloat(frame + 48u),
        runtime.ReadFloat(frame + 52u),
        runtime.ReadFloat(frame + 56u)};
    context.source_matrix = frame + 16u;
    runtime.SetupTransformMatrix(context.screen_matrix, frame + 16u);
    const auto reciprocal_view = 1.0f /
        runtime.ReadFloat(camera + 104u);
    runtime.ScaleMatrix(context.screen_matrix,
                        {-0.5f * reciprocal_view,
                         -0.5f * reciprocal_view, reciprocal_view}, 2u);
    runtime.TranslateMatrix(context.screen_matrix,
                            {0.5f, 0.5f, 0.0f}, 2u);
    context.intensity = static_cast<std::uint8_t>(
        runtime.Ftol(static_cast<double>(intensity) * 255.0));
    context.render_mode = render_mode;
    context.intersection = intersection;

    runtime.SetVertexCount(0u);
    runtime.TraverseIntersections(world, intersection, 0x5fe270u, context);
    const auto count = runtime.VertexCount();
    runtime.SetStatistic((count + 1500u * context.flush_count) / 3u);
    if (count != 0u) {
        if (runtime.TransformVertices(count, 0u, 25u) != 0u) {
            runtime.RenderPrimitive(3u);
            runtime.EndPrimitive();
        }
        runtime.SetVertexCount(0u);
    }
    (void)runtime.SetDeviceState(8u, 1u);
    (void)runtime.SetDeviceState(11u, 6u);
    (void)runtime.SetDeviceState(10u, 5u);
    return 1u;
}

std::uint32_t CameraCollisionRender::RenderGeometry(
    const std::uint32_t intersection, const std::uint32_t sector,
    const std::uint32_t triangle, const float distance,
    CollisionRenderContext& context,
    ICameraCollisionRenderRuntime& runtime) {
    (void)intersection;
    (void)sector;
    (void)distance;
    const auto dot =
        runtime.ReadFloat(triangle + 4u) * context.camera_position[1] +
        runtime.ReadFloat(triangle) * context.camera_position[0] +
        runtime.ReadFloat(triangle + 8u) * context.camera_position[2];
    if (dot > 0.0f) return triangle;

    std::array<std::array<float, 3u>, 3u> source{};
    for (std::size_t index = 0; index < source.size(); ++index) {
        const auto point = runtime.ReadWord(
            triangle + 28u + static_cast<std::uint32_t>(index * 4u));
        for (std::size_t axis = 0; axis < 3u; ++axis) {
            source[index][axis] = runtime.ReadFloat(
                point + static_cast<std::uint32_t>(axis * 4u));
        }
    }
    std::array<std::array<float, 3u>, 3u> projected{};
    runtime.TransformPoints(projected, source, context.screen_matrix);
    if (AllBelow(projected, 2u, 0.0f) ||
        AllBelow(projected, 0u, 0.0f) ||
        AllAbove(projected, 0u, 1.0f) ||
        AllBelow(projected, 1u, 0.0f) ||
        AllAbove(projected, 1u, 1.0f)) {
        return triangle;
    }

    const auto previous_state = runtime.GetRenderState(28u);
    runtime.SetRenderState(28u, 0u);
    auto count = runtime.VertexCount();
    if (count > 1497u) {
        if (runtime.TransformVertices(count, 0u, 25u) != 0u) {
            runtime.RenderPrimitive(3u);
            runtime.EndPrimitive();
        }
        count = 0u;
        ++context.flush_count;
        runtime.SetVertexCount(0u);
    }

    for (std::size_t index = 0; index < source.size(); ++index) {
        auto shade = context.intensity;
        if (context.render_mode != 0u) {
            const auto depth = projected[index][2];
            const auto factor = 1.0f - depth * depth;
            shade = factor >= 0.0f ?
                static_cast<std::uint8_t>(runtime.Ftol(
                    static_cast<double>(context.intensity) * factor)) :
                0u;
        }
        CollisionImmediateVertex vertex{};
        vertex.x = source[index][0];
        vertex.y = source[index][1] + 0.4f;
        vertex.z = source[index][2];
        vertex.color = ReplicateByte(shade);
        vertex.u = projected[index][0];
        vertex.v = projected[index][1];
        runtime.WriteVertex(count + static_cast<std::uint32_t>(index), vertex);
    }
    runtime.SetVertexCount(count + 3u);
    runtime.SetRenderState(28u, previous_state);
    return triangle;
}

}  // namespace pal4
