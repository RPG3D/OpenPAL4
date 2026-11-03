#include "pal4/pal_actor_control_floor_sound.h"

#include <bit>

namespace pal4 {

ActorControlFloorText IActorControlFloorSoundRuntime::CurrentFloorText() {
    return {};
}
std::uint32_t IActorControlFloorSoundRuntime::ClassifyFloorSound(
    const std::string_view path, PalActorControlState& state) {
    PalActorControlFloorEffectClassifier::Classify(path, state, *this);
    return 0u;
}

std::uint32_t PalActorControlFloorSound::Tick(
    PalActorControlState& state, const float delta_seconds,
    IActorControlFloorSoundRuntime& runtime) {
    if (state.words[1u] == 0u) {
        return 0u;
    }
    const auto total = static_cast<long double>(delta_seconds) +
        static_cast<long double>(std::bit_cast<float>(state.words[224u]));
    state.words[224u] = std::bit_cast<std::uint32_t>(
        static_cast<float>(total));
    if (!(total >= static_cast<long double>(0.2f))) {
        return 0x100u;
    }
    const auto floor = runtime.CurrentFloorText();
    if (floor.length == 0u) {
        return 0u;
    }
    state.words[224u] = 0u;
    return runtime.ClassifyFloorSound(floor.path, state);
}

}  // namespace pal4
