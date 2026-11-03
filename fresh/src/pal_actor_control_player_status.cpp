#include "pal4/pal_actor_control_player_status.h"

#include <bit>

namespace pal4 {

std::uint32_t IActorControlPlayerStatusRuntime::PlayerManager() {
    return 0u;
}
std::uint32_t IActorControlPlayerStatusRuntime::FindPlayer(
    std::uint32_t, std::uint32_t) {
    return 0u;
}
std::uint32_t IActorControlPlayerStatusRuntime::HandleDamageOrStatus(
    PalActorControlState&, float) {
    return 0u;
}

std::uint32_t PalActorControlPlayerStatus::ApplyIfPlayerExists(
    PalActorControlState& state, const float value,
    IActorControlPlayerStatusRuntime& runtime) {
    const auto player = runtime.FindPlayer(
        runtime.PlayerManager(), state.words[19u]);
    if (player == 0u) {
        return player;
    }
    return runtime.HandleDamageOrStatus(state, value);
}

std::uint32_t PalActorControlPlayerStatus::Start(
    PalActorControlState& state, const std::uint32_t first,
    const std::uint32_t second, const float value,
    IActorControlPlayerStatusRuntime& runtime) {
    if (state.words[188u] != 0u) {
        return state.words[188u];
    }
    const auto player = runtime.FindPlayer(
        runtime.PlayerManager(), state.words[19u]);
    if (player == 0u) {
        return player;
    }
    state.words[193u] = std::bit_cast<std::uint32_t>(value);
    state.words[188u] = 1u;
    state.words[189u] = first;
    state.words[190u] = 0u;
    state.words[191u] = second;
    state.words[192u] = 0u;
    state.words[201u] = PalActorControl::GetMode(state);
    return runtime.HandleDamageOrStatus(state, value);
}

}  // namespace pal4
