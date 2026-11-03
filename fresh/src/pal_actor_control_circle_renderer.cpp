#include "pal4/pal_actor_control_circle_renderer.h"

#include <cmath>

namespace pal4 {

void IActorControlCircleRuntime::SinCos(
    const float angle, float& sine, float& cosine) {
    sine = std::sin(angle);
    cosine = std::cos(angle);
}
void IActorControlCircleRuntime::SetRenderState(
    std::uint32_t, std::uint32_t) {}
std::uint32_t IActorControlCircleRuntime::Transform(
    const ActorControlCircleVertex*, std::uint32_t, std::uint32_t) {
    return 0u;
}
void IActorControlCircleRuntime::RenderPrimitive(std::uint32_t) {}
std::uint32_t IActorControlCircleRuntime::End3D() { return 0u; }

std::uint32_t PalActorControlCircleRenderer::Draw(
    const ActorControlCircularEffect& effect,
    IActorControlCircleRuntime& runtime) {
    std::array<ActorControlCircleVertex, kVertexCount> vertices{};
    constexpr double kPi = 3.141592653589793;
    for (std::size_t index = 0; index < vertices.size(); ++index) {
        const auto angle = static_cast<float>(
            static_cast<double>(index) * 0.1 * kPi);
        float sine = 0.0f;
        float cosine = 0.0f;
        runtime.SinCos(angle, sine, cosine);
        auto& vertex = vertices[index];
        vertex.x = cosine * effect.radius + effect.position.x;
        vertex.y = effect.position.y;
        vertex.z = sine * effect.radius + effect.position.z;
        vertex.color = kColor;
    }
    runtime.SetRenderState(6u, 1u);
    runtime.SetRenderState(1u, 0u);
    const auto transformed = runtime.Transform(
        vertices.data(), static_cast<std::uint32_t>(vertices.size()), 2u);
    if (transformed == 0u) {
        return transformed;
    }
    runtime.RenderPrimitive(2u);
    return runtime.End3D();
}

}  // namespace pal4
