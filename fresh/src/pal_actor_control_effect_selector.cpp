#include "pal4/game_vector_math.h"
#include "pal4/pal_actor_control_effect_selector.h"

#include <cmath>

namespace pal4 {

ActorControlEffectPoint IActorControlEffectSelectorRuntime::Vertex(
    ActorControlEffectObject&, std::int32_t) { return {}; }
ActorControlEffectPoint IActorControlEffectSelectorRuntime::ActorPosition() {
    return {};
}
float IActorControlEffectSelectorRuntime::VerticalTolerance() {
    return 0.0f;
}
float IActorControlEffectSelectorRuntime::Magnitude(
    ActorControlEffectPoint offset) {
    return static_cast<float>(GameVectorMath::ApproximateMagnitude(
        {offset.x,offset.y,offset.z}));
}
void IActorControlEffectSelectorRuntime::SelectEffect(
    ActorControlEffectObject&) {}

ActorControlEffectObject* PalActorControlEffectSelector::Visit(
    ActorControlEffectObject& effect, float& best_distance,
    IActorControlEffectSelectorRuntime& runtime) {
    const auto& words = effect.words;
    if (words[1] != 5u || words[19] == 0u || words[21] == 0u)
        return &effect;
    const auto byte_span = static_cast<std::int32_t>(
        words[22] - words[21]);
    const auto count = byte_span / 80;
    if (count <= 0) return &effect;

    bool found = false;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto vertex = runtime.Vertex(effect, index);
        const auto actor = runtime.ActorPosition();
        const auto tolerance = runtime.VerticalTolerance();
        if (tolerance < std::fabs(vertex.y - actor.y)) continue;
        const ActorControlEffectPoint planar{
            vertex.x - actor.x, 0.0f, vertex.z - actor.z};
        const auto distance_squared = runtime.Magnitude(planar);
        if (distance_squared < best_distance) {
            best_distance = distance_squared;
            found = true;
        }
    }
    if (found) runtime.SelectEffect(effect);
    return &effect;
}

}  // namespace pal4
