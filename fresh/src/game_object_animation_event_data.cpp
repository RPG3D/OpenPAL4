#include "pal4/game_object_animation_event_data.h"

#include <algorithm>
#include <cstring>

namespace pal4 {

AnimationEventData& GameObjectAnimationEventData::Reset(
    AnimationEventData& event) noexcept {
    for (const auto offset : {0u, 64u, 128u, 204u})
        event[offset] = std::byte{};
    constexpr std::uint32_t negative_one = 0xbf800000u;
    constexpr std::uint32_t zero = 0;
    std::memcpy(event.data() + 192, &negative_one, 4);
    std::memcpy(event.data() + 196, &zero, 4);
    std::memcpy(event.data() + 200, &negative_one, 4);
    return event;
}

std::uint32_t GameObjectAnimationEventData::NormalizeTime(
    AnimationEventData& event) noexcept {
    std::uint32_t trigger_bits = 0;
    constexpr std::uint32_t zero = 0;
    std::memcpy(&trigger_bits, event.data() + 192, 4);
    std::memcpy(event.data() + 196, &zero, 4);
    std::memcpy(event.data() + 200, &trigger_bits, 4);
    return trigger_bits;
}

AnimationEventData& GameObjectAnimationEventData::Copy(
    AnimationEventData& destination, AnimationEventData& scratch,
    const AnimationEventData& source,
    IGameObjectAnimationEventStringRuntime& runtime) {
    for (const auto offset : {0u, 64u, 128u}) {
        runtime.CopyStringField(
            std::span<std::byte, 64>(destination.data() + offset, 64),
            std::span<const std::byte, 64>(source.data() + offset, 64));
    }
    std::copy(source.begin() + 192, source.end(), destination.begin() + 192);
    scratch = destination;
    return scratch;
}

}  // namespace pal4
