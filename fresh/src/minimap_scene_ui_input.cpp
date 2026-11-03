#include "pal4/minimap_scene_ui_input.h"

#include <vector>

namespace pal4 {

std::uint8_t MinimapSceneUiInput::HandleSpecialKey(
    const std::uint32_t event_address,
    IMinimapSceneUiSpecialKeyRuntime& runtime) {
    if (runtime.ReadEventWord(event_address + 0x10u) != 33u) {
        return 0u;
    }
    const auto target = runtime.ActorControl();
    const auto source = runtime.ActorControl();
    runtime.SetControlWord(target, runtime.GetControlWord(source) == 0);
    return 1u;
}

std::uint8_t MinimapSceneUiInput::HandlePlayerInput(
    const MinimapSceneUiState& state,
    const std::uint32_t owner,
    const std::uint32_t event_address,
    IMinimapSceneUiInputRuntime& runtime) {
    const auto code = runtime.ReadEventWord(event_address + 0x10u);
    const auto adapter = owner + 0x28u;

    if (code == 1u || (code >= 59u && code <= 65u)) {
        if (code == 1u &&
            runtime.ReadWord(runtime.ArenaManager() + 0xc4u) != 1u) {
            return 0u;
        }
        runtime.SwitchSystemState(owner);
        runtime.DispatchEvent(runtime.FrameManager() + 0x104u,
                              kSystemEventKeyCell,
                              {code == 1u ? 1u : code - 58u, 0u});
        return 1u;
    }
    switch (code) {
    case 19u:
        runtime.AdvanceRunMode(owner, adapter);
        return 1u;
    case 50u:
        runtime.ToggleState(owner);
        return 1u;
    case 46u:
        if (runtime.WindowVisible(state.ReadWord(0x68u))) {
            runtime.ActivateSkill(owner);
        }
        return 1u;
    case 47u:
        if (runtime.WindowVisible(state.ReadWord(0x64u))) {
            runtime.UpdateSkillState(owner);
        }
        return 1u;
    case 15u:
        break;
    default:
        return 0u;
    }

    auto manager = runtime.PlayerManager();
    if (runtime.ActivePlayerCount(manager) < 2) {
        return 0u;
    }
    const auto linked = state.ReadWord(0xccu);
    if (linked != 0u && runtime.ReadWord(linked + 0x34u) != 2u) {
        return 0u;
    }

    std::vector<std::uint32_t> players;
    for (std::uint32_t index = 0; index < 4u; ++index) {
        manager = runtime.PlayerManager();
        const auto player = runtime.PlayerByIndex(manager, index);
        if (player == 0u || runtime.ReadWord(player + 0xb08u) == 0u) {
            continue;
        }
        if (players.size() == players.capacity()) {
            players.reserve(players.empty() ? 1u : players.size() * 2u);
        }
        players.push_back(player);
    }
    runtime.SortPlayers(players);
    if (players.size() < 2u) {
        return 0u;
    }

    const auto last_state = runtime.PlayerState(players.back());
    // IDA 0x47b063 uses the array's capacity pointer, not its end pointer.
    for (auto index = players.capacity() - players.size(); index > 1u; --index) {
        runtime.SetPlayerState(players[players.size() + index - 1u],
                               runtime.PlayerState(players[players.size() + index - 2u]));
    }
    runtime.SetPlayerState(players.front(), last_state);
    RefreshPlayers(owner, players[1], runtime);
    return 1u;
}

void MinimapSceneUiInput::RefreshPlayers(
    const std::uint32_t owner,
    const std::uint32_t player,
    IMinimapSceneUiPlayerRefreshRuntime& runtime) {
    const auto action = runtime.ReadWord(player + 0xa48u);
    runtime.UpdatePlayerStates(runtime.PlayerManager(), action);
    runtime.UpdatePlayerAndGameStates(owner);
}

}  // namespace pal4
