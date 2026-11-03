#include "pal4/pal_actor_control_foot_effect.h"
#include "pal4/floor_effect_kind.h"

namespace pal4 {

std::uint32_t IActorControlFootEffectRuntime::EffectTransform(
    std::uint32_t) { return 0u; }
std::uint32_t IActorControlFootEffectRuntime::CreateEffect(
    std::string_view, std::uint32_t, std::uint32_t, std::uint32_t) {
    return 0u;
}
void IActorControlFootEffectRuntime::UpdateEffect(
    std::uint32_t, std::uint32_t, std::uint8_t) {}
void IActorControlFootEffectRuntime::ReleaseEffect(std::uint32_t) {}

void PalActorControlFootEffect::Spawn(
    PalActorControlState& state, std::uint32_t mode,
    IActorControlFootEffectRuntime& runtime) {
    const auto& words = state.words;
    if (words[0] == 0u || words[1] == 0u || words[223] == 0u ||
        words[33] == 0u || words[28] == 0u || words[183] != 0u)
        return;

    const auto transform = runtime.EffectTransform(words[223]) + 48u;
    std::string_view name;
    switch (mode) {
    case 1u: name = "foot_2"; break;
    case 2u: name = "foot_1"; break;
    case 3u: name = "foot_0"; break;
    default: return;
    }

    const auto effect = runtime.CreateEffect(name, transform, 1u, 0u);
    struct EffectGuard {
        IActorControlFootEffectRuntime& runtime;
        std::uint32_t effect;
        ~EffectGuard() { runtime.ReleaseEffect(effect); }
    } guard{runtime, effect};
    runtime.UpdateEffect(effect, words[1] + 160u, 0u);
}

void PalActorControlFloorEffectClassifier::Classify(
    std::string_view path, PalActorControlState& state,
    IActorControlFootEffectRuntime& runtime) {
    const auto mode = FloorEffectKind(path);
    if (mode != 0u) PalActorControlFootEffect::Spawn(state, mode, runtime);
}

}  // namespace pal4
