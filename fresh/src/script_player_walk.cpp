#include "pal4/script_player_walk.h"

#include <string>

namespace pal4 {
namespace {

// Live IDA5E2099 / 5E2B55: both RunTo callbacks pass430C0000 (140),
// independently verified for indexed Player and named NPC handlers.
constexpr float kScriptRunSpeed = 140.0f;

char FoldAscii(char value) {
    return value >= 'A' && value <= 'Z' ?
        static_cast<char>(value + ('a' - 'A')) : value;
}

bool SameAction(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i)
        if (FoldAscii(left[i]) != FoldAscii(right[i])) return false;
    return true;
}

}  // namespace

std::uint32_t ScriptPlayerWalk::WalkTo(
    std::int32_t index, const PlayerWalkPoint& target,
    std::uint8_t wait_mode) {
    return MoveTo(index, target, wait_mode, 45.0f, "C02", 0u);
}

std::uint32_t ScriptPlayerWalk::RunTo(
    std::int32_t index, const PlayerWalkPoint& target,
    std::uint8_t wait_mode) {
    return MoveTo(index, target, wait_mode, kScriptRunSpeed, "C03", 0u);
}

std::uint32_t ScriptPlayerWalk::WalkBack(
    std::int32_t index, const PlayerWalkPoint& target,
    std::uint8_t wait_mode) {
    return MoveTo(index, target, wait_mode, 45.0f, "C02", 1u);
}

std::uint32_t ScriptPlayerWalk::WalkNpc(
    std::string_view npc_name, const PlayerWalkPoint& target,
    std::uint8_t wait_mode) {
    return MoveNpc(npc_name, target, wait_mode, 45.0f, "C02", 0u);
}

std::uint32_t ScriptPlayerWalk::RunNpc(
    std::string_view npc_name, const PlayerWalkPoint& target,
    std::uint8_t wait_mode) {
    return MoveNpc(npc_name, target, wait_mode, kScriptRunSpeed, "C03", 0u);
}

std::uint32_t ScriptPlayerWalk::BackNpc(
    std::string_view npc_name, const PlayerWalkPoint& target,
    std::uint8_t wait_mode) {
    return MoveNpc(npc_name, target, wait_mode, 45.0f, "C02", 1u);
}

std::uint32_t ScriptPlayerWalk::EndMoveIndexed(std::int32_t index) {
    const auto manager = runtime_.PlayerManager();
    return EndActor(runtime_.PlayerByIndex(manager, index), index,
                    NameSource::Index, {});
}

std::uint32_t ScriptPlayerWalk::EndMoveCurrent() {
    const auto manager = runtime_.PlayerManager();
    return EndActor(runtime_.CurrentPlayer(manager), 0,
                    NameSource::CurrentActor, {});
}

std::uint32_t ScriptPlayerWalk::EndMoveNpc(
    std::string_view npc_name) {
    const auto manager = runtime_.NpcManager();
    return EndActor(runtime_.NpcByName(manager, npc_name), 0,
                    NameSource::Npc, npc_name);
}

std::uint32_t ScriptPlayerWalk::MoveNpc(
    std::string_view npc_name, const PlayerWalkPoint& target,
    std::uint8_t wait_mode, float speed,
    std::string_view action, std::uint32_t movement_status) {
    const auto manager = runtime_.NpcManager();
    const auto actor = runtime_.NpcByName(manager, npc_name);
    return MoveActor(actor, 0, NameSource::Npc, npc_name,
                     target, wait_mode, speed, action, movement_status);
}

std::uint32_t ScriptPlayerWalk::WalkCurrent(
    const PlayerWalkPoint& target, std::uint8_t wait_mode) {
    const auto actor = runtime_.CurrentPlayer(runtime_.PlayerManager());
    return MoveActor(actor, 0, NameSource::CurrentActor, {},
                     target, wait_mode, 45.0f, "C02", 0u);
}

std::uint32_t ScriptPlayerWalk::MoveTo(
    std::int32_t index, const PlayerWalkPoint& target,
    std::uint8_t wait_mode, float speed,
    std::string_view action, std::uint32_t movement_status) {
    const auto manager = runtime_.PlayerManager();
    const auto actor = runtime_.PlayerByIndex(manager, index);
    return MoveActor(actor, index, NameSource::Index, {}, target,
                     wait_mode, speed, action, movement_status);
}

std::uint32_t ScriptPlayerWalk::MoveActor(
    std::uint32_t actor, std::int32_t index,
    NameSource name_source, std::string_view npc_name,
    const PlayerWalkPoint& target, std::uint8_t wait_mode,
    float speed, std::string_view action,
    std::uint32_t movement_status) {
    if (actor == 0u) return 0u;

    const auto from = runtime_.ConvertPoint(runtime_.ReadPosition(actor));
    auto to = runtime_.ConvertPoint(target);
    const auto route = runtime_.PlanRoute(
        runtime_.ArenaManager() + kRouteSourceOffset, to);
    if (route == 0u) return 0u;

    const auto movement = actor + kMovementOffset;
    if (!runtime_.StartMovement(movement, from, to, route, 0u))
        return runtime_.ShowPathError();
    runtime_.SetMovementSpeed(movement, speed);
    runtime_.SetMovementStatus(actor, movement_status);
    if (!SameAction(runtime_.ActionName(actor), action) &&
        !SameAction(runtime_.ActionName(actor), "Z02")) {
        if (runtime_.SetAction(actor, action) == 0)
            static_cast<void>(runtime_.SetAction(actor, "Z02"));
    }

    const auto name = HandlerName(actor, index, name_source, npc_name);
    return QueueHandler(actor, name, wait_mode);
}

std::uint32_t ScriptPlayerWalk::EndActor(
    std::uint32_t actor, std::int32_t index,
    NameSource name_source, std::string_view npc_name) {
    if (actor == 0u) return 0u;
    const auto name = HandlerName(actor, index, name_source, npc_name);
    return QueueHandler(actor, name, 1u);
}

std::uint32_t ScriptPlayerWalk::QueueHandler(
    std::uint32_t actor, std::string_view name,
    std::uint8_t wait_mode) {
    auto* state = runtime_.Allocate(sizeof(PlayerWalkWaitState));
    if (state != nullptr) {
        runtime_.Construct(*state, wait_mode,
                           runtime_.ScriptExecutor() + kQueueOffset);
        state->actor = actor;
        state->vtable_address = kVtable;
        state->phase = 1u;
        runtime_.AssignName(*state, name);
        runtime_.CleanupWeaponEffects(actor);
        runtime_.Flush(*state);
    }
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);
    return runtime_.CleanupName();
}

std::string ScriptPlayerWalk::HandlerName(
    std::uint32_t actor, std::int32_t index,
    NameSource name_source, std::string_view npc_name) {
    std::string label;
    switch (name_source) {
    case NameSource::Index:
        label = std::to_string(index);
        break;
    case NameSource::CurrentActor:
        label = runtime_.ActorName(actor);
        break;
    case NameSource::Npc:
        label = npc_name;
        break;
    }
    return label + "-Player-Movment";
}

void ScriptPlayerWalk::Update(PlayerWalkWaitState& state) {
    if (runtime_.MovementActive(state.actor + kMovementOffset)) return;
    if (runtime_.SetAction(state.actor, "C01") == 0)
        static_cast<void>(runtime_.SetAction(state.actor, "Z01"));
    runtime_.Close(state);
}

void ScriptPlayerWalk::Stop(PlayerWalkWaitState& state) {
    runtime_.StopMovement(state.actor + kMovementOffset);
    runtime_.Close(state);
}

}  // namespace pal4
