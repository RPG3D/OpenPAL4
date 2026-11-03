#include "pal4/game_object_animation_event_dispatcher.h"

namespace pal4 {

bool GameObjectAnimationEventDispatcher::UpdateTime(
    GameObjectEmbeddedAnimationState& state, const float time,
    IGameObjectAnimationEventDispatchRuntime& runtime) {
    auto& words = state.words;
    if (words[1] == 0 || words[3] == 0) return false;

    for (auto event = runtime.EventBegin(words[3]);
         event != runtime.EventEnd(words[3]); event += kEventStride) {
        if (!(time >= runtime.TriggerTime(event)) ||
            runtime.FiredFlag(event) != 0)
            continue;

        runtime.SetFiredFlag(event, 1);
        auto listener_slot = words[7];
        for (; listener_slot != words[8]; listener_slot += 4u) {
            const auto listener = runtime.ListenerAt(listener_slot);
            if (!runtime.MatchesMessage(listener, event + 0x80u)) continue;
            words[10] = listener;
            runtime.Dispatch(listener, state, event);
            break;
        }
        if (listener_slot == words[8] && words[11] != 0)
            runtime.Dispatch(words[11], state, event);
    }
    return true;
}

}  // namespace pal4
