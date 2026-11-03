#include "pal4/script_player_position.h"

#include <string>

namespace pal4 {

std::uint32_t ScriptPlayerPosition::Set(
    std::int32_t index, const PlayerPosition& position) {
    const auto manager = runtime_.PlayerManager();
    const auto actor = runtime_.PlayerByIndex(manager, index);
    if (actor == 0u) return 0u;

    return SetActor(actor, position, index, false);
}

std::uint32_t ScriptPlayerPosition::SetCurrent(
    const PlayerPosition& position) {
    const auto actor = runtime_.CurrentPlayer(runtime_.PlayerManager());
    if (actor == 0u) return 0u;
    return SetActor(actor, position, 0, true);
}

std::uint32_t ScriptPlayerPosition::SetNpc(
    std::string_view npc_name, const PlayerPosition& position) {
    const auto actor = runtime_.NpcByName(runtime_.NpcManager(), npc_name);
    if (actor == 0u) return 0u;
    runtime_.SetNpcPosition(actor, position, 0u);
    if (runtime_.SetNpcAction(actor, "C01") == 0)
        static_cast<void>(runtime_.SetNpcAction(actor, "Z01"));
    const auto name = std::string(npc_name) + "-Player-Movment";
    return QueuePosition(name);
}

std::uint32_t ScriptPlayerPosition::SetActor(
    std::uint32_t actor, const PlayerPosition& position,
    std::int32_t index, bool current) {
    runtime_.SetPosition(actor, position);
    if (current || actor == runtime_.CurrentPlayer(runtime_.PlayerManager())) {
        const auto first = runtime_.MovementProperty(actor) * 0.35f;
        const auto second = runtime_.MovementProperty(actor) * 2.53f;
        const auto movement = actor + kMovementOffset;
        runtime_.SelectAndUpdate(runtime_.MovementAdapter(), movement,
                                 first, second, 1u);
        if (runtime_.CheckActive(runtime_.MovementAdapter(), movement,
                                 first, second, 0u)) {
            runtime_.UpdateFloorColor();
            runtime_.UpdateGrid(movement, actor + kGridContextOffset);
        }
    }
    runtime_.SetAction(actor, "C01");

    const auto name = (current ? std::string(runtime_.ActorName(actor)) :
                                 std::to_string(index)) + "-Player-Movment";
    return QueuePosition(name);
}

std::uint32_t ScriptPlayerPosition::QueuePosition(
    std::string_view name) {
    auto* state = runtime_.Allocate(sizeof(PlayerPositionWaitState));
    if (state != nullptr) {
        runtime_.Construct(*state, 0u,
                           runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kVtable;
        state->enabled = 1u;
        state->phase = 1u;
        runtime_.AssignName(*state, name);
    }
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);
    if (state && completed_handlers_)
        completed_handlers_->AppendCompletedHandler(kVtable, 1, name);
    return runtime_.CleanupName();
}

}  // namespace pal4
