#include "pal4/game_object_animation_query.h"

namespace pal4 {

std::uint32_t GameObjectAnimationQuery::CurrentAnimationData(
    const GameObjectEmbeddedAnimationState& state) noexcept {
    return state.words[3] == 0 ? 0 : state.words[3] + 0x34u;
}

std::uint32_t GameObjectAnimationQuery::SelectedAnimationName(
    const GameObjectEmbeddedAnimationState& state,
    IGameObjectAnimationQueryRuntime& runtime) {
    return state.words[10] == 0
               ? runtime.EmptyErrorString()
               : runtime.SelectedObjectName(state.words[10]);
}

std::uint32_t GameObjectAnimationQuery::CurrentRecordName(
    const GameObjectEmbeddedAnimationState& state,
    IGameObjectAnimationQueryRuntime& runtime) {
    if (state.words[3] == 0) return runtime.EmptyErrorString();
    const auto name = runtime.RecordName(state.words[3]);
    return name == 0 ? runtime.EmptyLogMessage() : name;
}

bool GameObjectAnimationQuery::ProcessAnimations(
    GameObjectEmbeddedAnimationState& state,
    const AnimationVisit& callback, const std::uint32_t context,
    IGameObjectAnimationQueryRuntime& runtime) {
    if (runtime.AnimationCount(state) <= 0 || !callback) return false;
    for (std::int32_t index = 0;
         index < runtime.AnimationCount(state); ++index) {
        if (!callback(runtime.AnimationEntry(state, index), index, context))
            break;
    }
    return true;
}

std::uint32_t GameObjectAnimationQuery::FindEvent(
    GameObjectEmbeddedAnimationState& state,
    const std::uint32_t requested_name,
    IGameObjectAnimationQueryRuntime& runtime) {
    if (state.words[3] == 0) return 0;
    auto entry = runtime.EventBegin(state.words[3]);
    if (entry == runtime.EventEnd(state.words[3])) return 0;
    while (true) {
        if (runtime.CompareEventName(entry + 0x80u, requested_name) == 0)
            return entry;
        entry += kEventStride;
        if (entry == runtime.EventEnd(state.words[3])) return 0;
    }
}

}  // namespace pal4
