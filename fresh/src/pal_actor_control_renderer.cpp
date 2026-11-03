#include "pal4/pal_actor_control_renderer.h"
#include "pal4/pal_actor_control_circle_renderer.h"

#include <bit>
#include <initializer_list>

namespace pal4 {

void ActorControlRenderRuntimeStub::SetRenderState(
    std::uint32_t, std::uint32_t) {}
bool ActorControlRenderRuntimeStub::TransformLine(
    const std::uint32_t*, std::uint32_t, std::uint32_t) {
    return false;
}
void ActorControlRenderRuntimeStub::RenderIndexedPrimitive(
    std::uint32_t, const std::array<std::uint16_t, 2>&) {}
void ActorControlRenderRuntimeStub::End3D() {}
double ActorControlRenderRuntimeStub::ActorDoubleProperty(
    std::uint32_t) { return 0.0; }
ActorControlVector3 ActorControlRenderRuntimeStub::ActorPosition(
    std::uint32_t) { return {}; }
void ActorControlRenderRuntimeStub::RenderCircularEffect(
    const ActorControlCircularEffect& effect) {
    IActorControlCircleRuntime runtime;
    (void)PalActorControlCircleRenderer::Draw(effect, runtime);
}

void PalActorControlRenderer::Draw(
    PalActorControlState& state, ActorControlRenderGlobals& globals,
    IActorControlRenderRuntime& runtime) {
    auto& words = state.words;
    if (words[0] == 0u || words[6] == 0u || words[1] == 0u)
        return;

    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(6u, 0u);
    runtime.SetRenderState(1u, 0u);
    constexpr std::array<std::uint16_t, 2> kLineIndices{0u, 1u};
    for (const auto offset : {225u, 243u}) {
        if (runtime.TransformLine(words.data() + offset, 2u, 2u)) {
            runtime.RenderIndexedPrimitive(1u, kLineIndices);
            runtime.End3D();
        }
    }
    runtime.SetRenderState(6u, 1u);
    runtime.SetRenderState(7u, 2u);

    const auto actor = words[1];
    const auto half_height = runtime.ActorDoubleProperty(actor) * 0.5;
    const auto position = runtime.ActorPosition(actor);
    globals.upper.position = position;
    globals.lower.position = position;
    globals.upper.position.y = static_cast<float>(
        static_cast<double>(position.y) + 1.25 * half_height);
    globals.lower.position.y = static_cast<float>(
        static_cast<double>(position.y) + 0.75 * half_height);
    globals.upper.radius = std::bit_cast<float>(words[9]);
    globals.lower.radius = std::bit_cast<float>(words[9]);
    runtime.RenderCircularEffect(globals.upper);
    runtime.RenderCircularEffect(globals.lower);
}

}  // namespace pal4
