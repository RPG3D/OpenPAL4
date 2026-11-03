#include "pal4/script_player_effects.h"

namespace pal4 {

std::uint32_t ScriptPlayerEffects::Hook(
    const std::int32_t player_index,
    const std::uint32_t effect_record,
    const std::int32_t argument,
    IScriptPlayerEffectsRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(
        runtime.PlayerManager(), player_index);
    if (player == 0u) return 0u;
    auto effect = runtime.ReadWord(effect_record + 4u);
    if (effect == 0u) effect = runtime.EmptyString();
    return runtime.AttachEffect(player, effect, argument);
}

std::uint32_t ScriptPlayerEffects::Detach(
    const std::int32_t player_index,
    IScriptPlayerEffectsRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(
        runtime.PlayerManager(), player_index);
    if (player == 0u) return 0u;
    return runtime.DetachEffect(player);
}

}  // namespace pal4
