#include "pal4/actor_animation_events.h"

namespace pal4 {

std::uint32_t ActorAnimationEvents::CompareNameAndFlag(
    const std::uint32_t object, const std::uint32_t search,
    IActorAnimationEventRuntime& runtime) {
    const auto object_text = runtime.ReadWord(object + 20u);
    const auto search_text = runtime.ReadWord(search + 4u);
    const auto left = object_text ? runtime.TextAt(object_text) :
                                    std::string_view{};
    const auto right = search_text ? runtime.TextAt(search_text) :
                                      std::string_view{};
    if (!runtime.EqualIgnoringCase(left, right)) return object;
    runtime.SetAnimationFound(true);
    return 0;
}

std::uint32_t ActorAnimationEvents::ProcessMessage(
    const std::uint32_t object, const std::uint32_t,
    const std::uint32_t message, IActorAnimationEventRuntime& runtime) {
    const auto actor = runtime.ReadWord(object + 4u);
    if (!actor) return 1;
    const std::array<std::uint32_t, 2> payload{actor + 716u, message};
    const auto key = runtime.ReadWord(0x8b595cu);
    return runtime.Dispatch(runtime.EventRouter(), key, payload, 1u);
}

}  // namespace pal4
